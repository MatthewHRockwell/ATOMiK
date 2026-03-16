import { randomBytes } from 'crypto';

export interface License {
  key: string;
  email: string;
  tier: string;
  stripeSessionId: string;
  createdAt: string;
}

/**
 * Generate a cryptographically random license key.
 * Format: ATOMIK-XXXX-XXXX-XXXX-XXXX (4 groups of 4 uppercase hex chars)
 */
export function generateSecureLicenseKey(): string {
  const bytes = randomBytes(8); // 8 bytes = 16 hex chars
  const hex = bytes.toString('hex').toUpperCase();
  const parts = [
    hex.slice(0, 4),
    hex.slice(4, 8),
    hex.slice(8, 12),
    hex.slice(12, 16),
  ];
  return `ATOMIK-${parts.join('-')}`;
}
