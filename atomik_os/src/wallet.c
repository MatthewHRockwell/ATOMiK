/* wallet.c — local token wallet + visible cost ledger.
 *
 * v0.15 makes the BUSINESS_MODEL.md pitch tangible: a balance the user
 * explicitly funds, a daily cap, an "amount spent so far" ledger pulled
 * from the LLM audit log, and a per-action affordability check that
 * any code path can use BEFORE firing a paid LLM call.
 *
 * Persisted to /tmp/atomik_os_wallet.state. v1.0 moves to /var. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define WALLET_PATH  "/tmp/atomik_os_wallet.state"
#define WALLET_MAGIC 0xA01D5741u   /* "ATOMiK wlt v1" */

static wallet_state_t s_w = {
    .balance_uusd     = 5000000,    /* $5.00 starter balance */
    .spent_today_uusd = 0,
    .daily_cap_uusd   = 1000000,    /* $1.00/day soft cap by default */
    .last_reset_day   = 0,
};
static int s_loaded = 0;

static int unix_day(void) {
    return (int)(time(NULL) / 86400);
}

void wallet_save(void) {
    FILE *f = fopen(WALLET_PATH, "wb");
    if (!f) return;
    uint32_t magic = WALLET_MAGIC;
    fwrite(&magic, sizeof magic, 1, f);
    fwrite(&s_w,   sizeof s_w,   1, f);
    fclose(f);
}

void wallet_init(void) {
    if (s_loaded) return;
    s_loaded = 1;
    FILE *f = fopen(WALLET_PATH, "rb");
    if (f) {
        uint32_t magic = 0;
        if (fread(&magic, sizeof magic, 1, f) == 1 && magic == WALLET_MAGIC) {
            (void)!fread(&s_w, sizeof s_w, 1, f);
        }
        fclose(f);
    }
    /* Daily roll: if a new day, reset spent_today. */
    int today = unix_day();
    if (today != s_w.last_reset_day) {
        s_w.spent_today_uusd = 0;
        s_w.last_reset_day   = today;
        wallet_save();
    }
}

const wallet_state_t *wallet_get(void) { wallet_init(); return &s_w; }

int wallet_topup(int uusd) {
    wallet_init();
    if (uusd <= 0) return 0;
    s_w.balance_uusd += uusd;
    wallet_save();
    return s_w.balance_uusd;
}

int wallet_can_afford(int uusd) {
    wallet_init();
    if (uusd <= 0) return 1;
    if (uusd > s_w.balance_uusd) return 0;
    if (s_w.daily_cap_uusd > 0 &&
        s_w.spent_today_uusd + uusd > s_w.daily_cap_uusd) return 0;
    return 1;
}

int wallet_charge(int uusd) {
    wallet_init();
    if (!wallet_can_afford(uusd)) return 0;
    s_w.balance_uusd     -= uusd;
    s_w.spent_today_uusd += uusd;
    wallet_save();
    return 1;
}

void wallet_set_daily_cap(int uusd) {
    wallet_init();
    s_w.daily_cap_uusd = uusd < 0 ? 0 : uusd;
    wallet_save();
}

/* ---- Wallet app ---- */

static void format_usd(char *out, size_t cap, int uusd) {
    int sign = uusd < 0 ? -1 : 1;
    if (uusd < 0) uusd = -uusd;
    snprintf(out, cap, "%s$%d.%06d",
             sign < 0 ? "-" : "",
             uusd / 1000000, uusd % 1000000);
}

void wallet_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    wallet_init();

    draw_rect(x, y, wd, ht, rgb(0x10, 0x16, 0x22));
    draw_text(x + 24, y + 16, "Wallet", 3, ATOMIK_FG);

    int row_y = y + 80;
    int row_h = 36;

    char buf[128], usd[64];

    format_usd(usd, sizeof usd, s_w.balance_uusd);
    snprintf(buf, sizeof buf, "Balance: %s", usd);
    draw_text(x + 24, row_y, buf, 2, ATOMIK_ACCENT);
    row_y += row_h * 2;

    int total = llm_audit_total_uusd();
    format_usd(usd, sizeof usd, total);
    snprintf(buf, sizeof buf, "Lifetime spent (audit log): %s", usd);
    draw_text(x + 24, row_y, buf, 1, ATOMIK_FG);
    row_y += row_h;

    format_usd(usd, sizeof usd, s_w.spent_today_uusd);
    snprintf(buf, sizeof buf, "Spent today: %s", usd);
    draw_text(x + 24, row_y, buf, 1, ATOMIK_FG);
    row_y += row_h;

    format_usd(usd, sizeof usd, s_w.daily_cap_uusd);
    snprintf(buf, sizeof buf, "Daily cap:   %s", usd);
    draw_text(x + 24, row_y, buf, 1, ATOMIK_FG_DIM);
    row_y += row_h * 2;

    /* Audit log tail */
    draw_text(x + 24, row_y, "Recent activity:", 1, ATOMIK_ACCENT);
    row_y += row_h - 8;

    FILE *f = fopen("/tmp/atomik_os_llm_audit.log", "r");
    if (f) {
        /* Read last ~1.5 KB so we render the tail */
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        long start = sz > 1500 ? sz - 1500 : 0;
        fseek(f, start, SEEK_SET);
        char line[200];
        int rendered = 0, max_rows = (ht - (row_y - y) - 24) / 18;
        if (max_rows < 1) max_rows = 1;
        /* Skip the partial first line */
        if (start > 0) (void)!fgets(line, sizeof line, f);
        while (fgets(line, sizeof line, f) && rendered < max_rows) {
            line[strcspn(line, "\r\n")] = 0;
            draw_text(x + 24, row_y, line, 1, ATOMIK_FG_DIM);
            row_y += 18;
            rendered++;
        }
        fclose(f);
        if (rendered == 0) {
            draw_text(x + 24, row_y, "(no spends yet)", 1, ATOMIK_FG_DIM);
        }
    } else {
        draw_text(x + 24, row_y, "(no audit log yet)", 1, ATOMIK_FG_DIM);
    }

    draw_text(x + 24, y + ht - 22,
              "wallet persisted at /tmp/atomik_os_wallet.state",
              1, ATOMIK_FG_DIM);
}
