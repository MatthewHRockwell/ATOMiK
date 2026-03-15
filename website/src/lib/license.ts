import { createHash, randomBytes } from 'crypto';

const LICENSE_SECRET = process.env.LICENSE_SECRET || 'atomik-dev-secret';

export interface LicenseData {
  key: string;
  email: string;
  tier: 'professional' | 'enterprise';
  createdAt: string;
  expiresAt: string;
  stripeCustomerId: string;
  stripeSubscriptionId: string;
}

/**
 * Generate a license key from subscription data.
 * Format: ATOMIK-XXXX-XXXX-XXXX-XXXX
 */
export function generateLicenseKey(
  email: string,
  tier: string,
  subscriptionId: string
): string {
  const seed = `${email}:${tier}:${subscriptionId}:${LICENSE_SECRET}`;
  const hash = createHash('sha256').update(seed).digest('hex');
  const parts = [
    hash.slice(0, 4).toUpperCase(),
    hash.slice(4, 8).toUpperCase(),
    hash.slice(8, 12).toUpperCase(),
    hash.slice(12, 16).toUpperCase(),
  ];
  return `ATOMIK-${parts.join('-')}`;
}

/**
 * Validate a license key format.
 */
export function isValidKeyFormat(key: string): boolean {
  return /^ATOMIK-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}$/.test(key);
}

/**
 * Generate a signed download token (expires in 1 hour).
 */
export function generateDownloadToken(licenseKey: string, platform: string): string {
  const expires = Date.now() + 3600000; // 1 hour
  const payload = `${licenseKey}:${platform}:${expires}`;
  const sig = createHash('sha256')
    .update(`${payload}:${LICENSE_SECRET}`)
    .digest('hex')
    .slice(0, 16);
  return Buffer.from(`${payload}:${sig}`).toString('base64url');
}

/**
 * Verify a download token.
 */
export function verifyDownloadToken(token: string): { licenseKey: string; platform: string } | null {
  try {
    const decoded = Buffer.from(token, 'base64url').toString();
    const parts = decoded.split(':');
    if (parts.length !== 4) return null;

    const [licenseKey, platform, expiresStr, sig] = parts;
    const expires = parseInt(expiresStr, 10);

    if (Date.now() > expires) return null;

    const expectedSig = createHash('sha256')
      .update(`${licenseKey}:${platform}:${expiresStr}:${LICENSE_SECRET}`)
      .digest('hex')
      .slice(0, 16);

    if (sig !== expectedSig) return null;

    return { licenseKey, platform };
  } catch {
    return null;
  }
}
