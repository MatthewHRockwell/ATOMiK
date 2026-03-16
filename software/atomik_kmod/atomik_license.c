// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_license.c — License validation for ATOMiK kernel module
 *
 * Supports:
 *   - No key: 90-day trial (start date persisted in /var/lib/atomik/trial_start)
 *   - Valid key: HMAC-SHA256 verified, perpetual license
 *   - Expired trial: module loads but operations return -EPERM
 *
 * License key format:
 *   ATOMIK-XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX  (40 hex chars)
 *
 *   Bytes 0-3:   payload (version, tier, reserved)
 *   Bytes 4-19:  HMAC-SHA256(secret, payload) truncated to 128 bits
 *
 * Keys are generated server-side with tools/atomik-keygen.py.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <crypto/hash.h>
#include <crypto/utils.h>

#define ATOMIK_TRIAL_DAYS	90
#define ATOMIK_TRIAL_PATH	"/var/lib/atomik/trial_start"
#define ATOMIK_KEY_PREFIX	"ATOMIK-"
#define ATOMIK_KEY_LEN		51	/* ATOMIK-XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX */
#define ATOMIK_KEY_PAYLOAD_LEN	4
#define ATOMIK_KEY_HMAC_LEN	16	/* 128-bit truncated HMAC */
#define ATOMIK_KEY_DECODED_LEN	20	/* 4 payload + 16 HMAC */

/* License tiers */
#define ATOMIK_TIER_PROFESSIONAL	0x01
#define ATOMIK_TIER_ENTERPRISE		0x02

/*
 * HMAC signing key — used to verify license keys offline.
 *
 * This key is embedded in the compiled module. While a determined
 * attacker could extract it via binary analysis, this provides
 * sufficient protection against casual piracy. Server-side license
 * validation (phone-home) can be added in a future version for
 * enterprise deployments.
 *
 * This key MUST match the key used by tools/atomik-keygen.py.
 */
static const u8 hmac_secret[32] = {
	0xa7, 0x3c, 0x19, 0x8e, 0x42, 0xf5, 0xd1, 0x6b,
	0x90, 0x2d, 0xe8, 0x54, 0xc3, 0x7a, 0x0f, 0xb6,
	0x5e, 0x81, 0x3d, 0xf9, 0x27, 0x64, 0xab, 0xc0,
	0x48, 0x95, 0x1e, 0xd2, 0x73, 0xba, 0x06, 0x5f
};

/* Forward declarations */
int atomik_license_days_remaining(void);

static bool licensed;
static u8 license_tier;
static bool trial_active;
static time64_t trial_start;

/* --- Hex decoding --- */

static int hex_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

static int hex_decode(const char *hex, u8 *out, int out_len)
{
	int i;

	for (i = 0; i < out_len; i++) {
		int hi = hex_nibble(hex[2 * i]);
		int lo = hex_nibble(hex[2 * i + 1]);

		if (hi < 0 || lo < 0)
			return -EINVAL;
		out[i] = (hi << 4) | lo;
	}
	return 0;
}

/* --- HMAC-SHA256 verification --- */

static int verify_hmac(const u8 *payload, int payload_len,
		       const u8 *expected_hmac, int hmac_len)
{
	struct crypto_shash *tfm;
	struct shash_desc *desc;
	u8 digest[32]; /* SHA-256 output */
	int ret;

	tfm = crypto_alloc_shash("hmac(sha256)", 0, 0);
	if (IS_ERR(tfm)) {
		pr_err("atomik: crypto_alloc_shash failed: %ld\n",
		       PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	ret = crypto_shash_setkey(tfm, hmac_secret, sizeof(hmac_secret));
	if (ret) {
		pr_err("atomik: crypto_shash_setkey failed: %d\n", ret);
		goto out_free_tfm;
	}

	desc = kzalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
	if (!desc) {
		ret = -ENOMEM;
		goto out_free_tfm;
	}
	desc->tfm = tfm;

	ret = crypto_shash_digest(desc, payload, payload_len, digest);
	if (ret) {
		pr_err("atomik: crypto_shash_digest failed: %d\n", ret);
		goto out_free_desc;
	}

	/* Constant-time comparison of truncated HMAC */
	ret = crypto_memneq(digest, expected_hmac, hmac_len) ? -EACCES : 0;

out_free_desc:
	kfree(desc);
out_free_tfm:
	crypto_free_shash(tfm);
	return ret;
}

/* --- License key validation --- */

static int validate_license_key(const char *key)
{
	char hex_buf[64];
	u8 decoded[ATOMIK_KEY_DECODED_LEN];
	int ret, j = 0, i;

	if (!key || strlen(key) != ATOMIK_KEY_LEN)
		return -EINVAL;

	if (strncmp(key, ATOMIK_KEY_PREFIX, 7) != 0)
		return -EINVAL;

	/* Extract hex characters, skipping dashes at positions 15, 24, 33, 42 */
	for (i = 7; i < ATOMIK_KEY_LEN; i++) {
		if (key[i] == '-') {
			if (i != 15 && i != 24 && i != 33 && i != 42)
				return -EINVAL;
			continue;
		}
		if (j >= 40) /* 20 bytes * 2 hex chars */
			return -EINVAL;
		hex_buf[j++] = key[i];
	}

	if (j != 40)
		return -EINVAL;

	hex_buf[j] = '\0';

	/* Decode hex → bytes */
	ret = hex_decode(hex_buf, decoded, ATOMIK_KEY_DECODED_LEN);
	if (ret)
		return ret;

	/* Verify payload format */
	if (decoded[0] != 0x01) {
		pr_warn("atomik: unsupported key version 0x%02x\n", decoded[0]);
		return -EINVAL;
	}

	if (decoded[1] != ATOMIK_TIER_PROFESSIONAL &&
	    decoded[1] != ATOMIK_TIER_ENTERPRISE) {
		pr_warn("atomik: invalid license tier 0x%02x\n", decoded[1]);
		return -EINVAL;
	}

	/* Verify HMAC signature */
	ret = verify_hmac(decoded, ATOMIK_KEY_PAYLOAD_LEN,
			  decoded + ATOMIK_KEY_PAYLOAD_LEN,
			  ATOMIK_KEY_HMAC_LEN);
	if (ret) {
		pr_warn("atomik: license key signature verification failed\n");
		return -EACCES;
	}

	license_tier = decoded[1];
	return 0;
}

/* --- Trial persistence --- */

static time64_t read_trial_start(void)
{
	struct file *f;
	char buf[32];
	loff_t pos = 0;
	ssize_t n;
	time64_t ts = 0;
	int i;

	f = filp_open(ATOMIK_TRIAL_PATH, O_RDONLY, 0);
	if (IS_ERR(f))
		return 0; /* file doesn't exist yet */

	n = kernel_read(f, buf, sizeof(buf) - 1, &pos);
	filp_close(f, NULL);

	if (n <= 0)
		return 0;

	buf[n] = '\0';

	/* Parse decimal timestamp */
	for (i = 0; i < n && buf[i] >= '0' && buf[i] <= '9'; i++)
		ts = ts * 10 + (buf[i] - '0');

	return ts;
}

static int write_trial_start(time64_t ts)
{
	struct file *f;
	char buf[32];
	loff_t pos = 0;
	int len;

	len = snprintf(buf, sizeof(buf), "%lld\n", ts);

	f = filp_open(ATOMIK_TRIAL_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (IS_ERR(f)) {
		pr_warn("atomik: cannot write trial state to %s: %ld\n",
			ATOMIK_TRIAL_PATH, PTR_ERR(f));
		return PTR_ERR(f);
	}

	kernel_write(f, buf, len, &pos);
	filp_close(f, NULL);
	return 0;
}

/* --- Public interface --- */

int atomik_license_init(const char *key)
{
	time64_t now = ktime_get_real_seconds();
	int ret;

	/* Check for valid license key */
	if (key && key[0] != '\0') {
		ret = validate_license_key(key);
		if (ret == 0) {
			licensed = true;
			trial_active = false;
			pr_info("atomik: license verified (%s)\n",
				license_tier == ATOMIK_TIER_ENTERPRISE ?
				"enterprise" : "professional");
			return 0;
		}
		if (ret == -EACCES) {
			pr_err("atomik: invalid license key — "
			       "signature verification failed\n");
			return -EINVAL;
		}
		pr_warn("atomik: malformed license key\n");
		return -EINVAL;
	}

	/* No key — start or resume trial */
	licensed = false;
	trial_active = true;

	trial_start = read_trial_start();
	if (trial_start == 0) {
		/* First boot — start new trial */
		trial_start = now;
		write_trial_start(trial_start);
		pr_info("atomik: 90-day trial started (%d days remaining)\n",
			ATOMIK_TRIAL_DAYS);
	} else {
		/* Resuming existing trial */
		int days = atomik_license_days_remaining();

		if (days > 0)
			pr_info("atomik: trial resumed (%d days remaining)\n",
				days);
		else
			pr_warn("atomik: trial expired — "
				"operations will return -EPERM\n");
	}

	return 0;
}

void atomik_license_exit(void)
{
	/* Nothing to clean up */
}

int atomik_license_check(void)
{
	return licensed ? 1 : 0;
}

int atomik_license_days_remaining(void)
{
	time64_t now, elapsed;
	int days;

	if (licensed)
		return -1; /* perpetual */

	if (!trial_active)
		return 0;

	now = ktime_get_real_seconds();
	elapsed = now - trial_start;
	days = ATOMIK_TRIAL_DAYS - (int)div64_long(elapsed, 86400);

	return (days > 0) ? days : 0;
}

bool atomik_license_is_active(void)
{
	if (licensed)
		return true;
	return atomik_license_days_remaining() > 0;
}
