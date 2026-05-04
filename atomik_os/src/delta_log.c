/* delta_log.c — wire format for edge-app field deltas.
 *
 * Encodes ATOMiK OS's "every mutation is a delta" architecture into a
 * versioned, byte-stream-friendly format. Same code path serves on-disk
 * persistence and over-the-wire streaming — the latter when v1.0 wires
 * networking. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

static FILE *s_log = NULL;

static int wbe16(FILE *f, uint16_t v) {
    uint8_t b[2] = { v >> 8, v & 0xff };
    return fwrite(b, 1, 2, f) == 2 ? 0 : -1;
}

static int wbe32(FILE *f, uint32_t v) {
    uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v };
    return fwrite(b, 1, 4, f) == 4 ? 0 : -1;
}

static int wu8(FILE *f, uint8_t v) {
    return fwrite(&v, 1, 1, f) == 1 ? 0 : -1;
}

static int wstr(FILE *f, const char *s) {
    size_t n = s ? strlen(s) : 0;
    if (n > 0xFFFF) n = 0xFFFF;
    if (wbe16(f, (uint16_t)n) < 0) return -1;
    if (n && fwrite(s, 1, n, f) != n) return -1;
    return 0;
}

int delta_log_open(const char *path) {
    s_log = fopen(path, "wb");
    if (!s_log) return -1;
    if (wbe32(s_log, DELTA_LOG_MAGIC) < 0) { fclose(s_log); s_log = NULL; return -1; }
    if (wbe32(s_log, DELTA_LOG_VER)   < 0) { fclose(s_log); s_log = NULL; return -1; }
    return 0;
}

void delta_log_close(void) {
    if (s_log) { fflush(s_log); fclose(s_log); s_log = NULL; }
}

int delta_emit_set_primitive(primitive_t p) {
    if (!s_log) return -1;
    if (wu8(s_log, OP_SET_PRIMITIVE) < 0) return -1;
    return wu8(s_log, (uint8_t)p);
}

int delta_emit_set_accent(pixel_t accent) {
    if (!s_log) return -1;
    if (wu8(s_log, OP_SET_ACCENT) < 0) return -1;
    return wbe32(s_log, accent);
}

int delta_emit_set_field_str(int field_id, const char *s) {
    if (!s_log) return -1;
    if (wu8(s_log, OP_SET_FIELD_STR) < 0) return -1;
    if (wu8(s_log, (uint8_t)field_id) < 0) return -1;
    return wstr(s_log, s);
}

int delta_emit_list_append(int field_id, const char *s) {
    if (!s_log) return -1;
    if (wu8(s_log, OP_LIST_APPEND) < 0) return -1;
    if (wu8(s_log, (uint8_t)field_id) < 0) return -1;
    return wstr(s_log, s);
}

int delta_emit_list_clear(int field_id) {
    if (!s_log) return -1;
    if (wu8(s_log, OP_LIST_CLEAR) < 0) return -1;
    return wu8(s_log, (uint8_t)field_id);
}

int delta_emit_reset(void) {
    if (!s_log) return -1;
    return wu8(s_log, OP_RESET);
}

int delta_emit_set_name(const char *s) {
    if (!s_log) return -1;
    if (wu8(s_log, OP_SET_NAME) < 0) return -1;
    return wstr(s_log, s);
}

int delta_emit_set_subtitle(const char *s) {
    if (!s_log) return -1;
    if (wu8(s_log, OP_SET_SUBTITLE) < 0) return -1;
    return wstr(s_log, s);
}

/* ---- Replay ---- */

static int rbe16(FILE *f, uint16_t *out) {
    uint8_t b[2];
    if (fread(b, 1, 2, f) != 2) return -1;
    *out = (b[0] << 8) | b[1];
    return 0;
}
static int rbe32(FILE *f, uint32_t *out) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return -1;
    *out = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8)  | b[3];
    return 0;
}
static int ru8(FILE *f, uint8_t *out) {
    return fread(out, 1, 1, f) == 1 ? 0 : -1;
}
static int rstr(FILE *f, char *buf, size_t cap) {
    uint16_t n;
    if (rbe16(f, &n) < 0) return -1;
    if (n >= cap) return -1;
    if (n && fread(buf, 1, n, f) != n) return -1;
    buf[n] = 0;
    return 0;
}

int delta_replay_file(const char *path, edge_app_t *a) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    uint32_t magic, ver;
    if (rbe32(f, &magic) < 0 || magic != DELTA_LOG_MAGIC ||
        rbe32(f, &ver)   < 0 || ver   != DELTA_LOG_VER) {
        fclose(f); return -1;
    }
    int applied = 0;
    char buf[FIELD_STR_MAX];
    for (;;) {
        uint8_t op;
        if (ru8(f, &op) < 0) break;   /* EOF */
        switch (op) {
            case OP_SET_PRIMITIVE: {
                uint8_t p; if (ru8(f, &p) < 0) goto done;
                a->primitive = (primitive_t)p;
                break;
            }
            case OP_SET_ACCENT: {
                uint32_t c; if (rbe32(f, &c) < 0) goto done;
                a->accent = c;
                break;
            }
            case OP_SET_FIELD_STR: {
                uint8_t id; if (ru8(f, &id) < 0) goto done;
                if (rstr(f, buf, sizeof buf) < 0) goto done;
                eapp_set_str(a, id, buf);
                break;
            }
            case OP_LIST_APPEND: {
                uint8_t id; if (ru8(f, &id) < 0) goto done;
                if (rstr(f, buf, sizeof buf) < 0) goto done;
                eapp_list_append(a, id, buf);
                break;
            }
            case OP_LIST_CLEAR: {
                uint8_t id; if (ru8(f, &id) < 0) goto done;
                eapp_clear_list(a, id);
                break;
            }
            case OP_RESET: {
                /* Wipe field values, preserve schema (counts + types). */
                for (int i = 0; i < a->field_count; i++) {
                    a->fields[i].str[0] = 0;
                    a->fields[i].i      = 0;
                    a->fields[i].list_n = 0;
                    a->fields[i].color  = 0;
                }
                break;
            }
            case OP_SET_NAME: {
                if (rstr(f, buf, sizeof buf) < 0) goto done;
                snprintf(a->name, sizeof a->name, "%s", buf);
                break;
            }
            case OP_SET_SUBTITLE: {
                if (rstr(f, buf, sizeof buf) < 0) goto done;
                snprintf(a->subtitle, sizeof a->subtitle, "%s", buf);
                break;
            }
            default:
                /* Unknown opcode — abort replay rather than misinterpret. */
                goto done;
        }
        applied++;
    }
done:
    fclose(f);
    return applied;
}

int delta_snapshot_to_file(const char *path, const edge_app_t *a) {
    if (delta_log_open(path) < 0) return -1;
    /* RESET first so the consumer starts from a known empty state. */
    delta_emit_reset();
    delta_emit_set_name(a->name);
    delta_emit_set_subtitle(a->subtitle);
    delta_emit_set_primitive(a->primitive);
    delta_emit_set_accent(a->accent);
    for (int i = 0; i < a->field_count; i++) {
        const field_value_t *fv = &a->fields[i];
        switch (fv->type) {
            case FT_STR:
                delta_emit_set_field_str(i, fv->str);
                break;
            case FT_LIST:
                delta_emit_list_clear(i);
                for (int j = 0; j < fv->list_n; j++)
                    delta_emit_list_append(i, fv->list[j]);
                break;
            default:
                /* INT and COLOR not currently emitted — extend opcodes
                 * when fields of those types matter for a primitive. */
                break;
        }
    }
    delta_log_close();
    return 0;
}
