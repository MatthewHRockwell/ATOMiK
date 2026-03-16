import { randomBytes } from 'crypto';

export interface License {
  key: string;
  email: string;
  tier: string;
  stripeSessionId: string;
  createdAt: string;
}

/**
 * Generate a cryptographically random license key with expiry.
 * Format: ATOMIK-XXXX-XXXX-XXXX-YYMM
 *   - First 3 groups: random hex (12 chars of entropy)
 *   - Last group: expiry as YYMM (2-digit year + 2-digit month, hex-encoded)
 *   - The kernel module checks if current date > YYMM and rejects expired keys.
 *   - Subscriptions are monthly, so keys expire at end of the billing month.
 *
 * @param monthsValid How many months the key is valid (default 2 for buffer)
 */
export function generateSecureLicenseKey(monthsValid: number = 2): string {
  const bytes = randomBytes(6); // 6 bytes = 12 hex chars for first 3 groups
  const hex = bytes.toString('hex').toUpperCase();

  // Calculate expiry: current month + monthsValid
  const now = new Date();
  const expiryDate = new Date(now.getFullYear(), now.getMonth() + monthsValid, 1);
  const yy = (expiryDate.getFullYear() % 100).toString(16).toUpperCase().padStart(2, '0');
  const mm = (expiryDate.getMonth() + 1).toString(16).toUpperCase().padStart(2, '0');
  const expiry = `${yy}${mm}`;

  const parts = [
    hex.slice(0, 4),
    hex.slice(4, 8),
    hex.slice(8, 12),
    expiry,
  ];
  return `ATOMIK-${parts.join('-')}`;
}
