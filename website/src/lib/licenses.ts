import { randomBytes } from 'crypto';
import { readFile, writeFile, mkdir } from 'fs/promises';
import path from 'path';

const DATA_DIR = path.join(process.cwd(), 'data');
const LICENSES_FILE = path.join(DATA_DIR, 'licenses.json');

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

async function readLicenses(): Promise<License[]> {
  try {
    const data = await readFile(LICENSES_FILE, 'utf-8');
    return JSON.parse(data);
  } catch {
    return [];
  }
}

async function writeLicenses(licenses: License[]): Promise<void> {
  await mkdir(DATA_DIR, { recursive: true });
  await writeFile(LICENSES_FILE, JSON.stringify(licenses, null, 2));
}

/**
 * Save a license to the JSON store.
 * If a license with the same session ID already exists, return the existing one
 * (idempotent -- safe for webhook retries).
 */
export async function saveLicense(license: License): Promise<License> {
  const licenses = await readLicenses();

  const existing = licenses.find((l) => l.stripeSessionId === license.stripeSessionId);
  if (existing) {
    return existing;
  }

  licenses.push(license);
  await writeLicenses(licenses);
  return license;
}

/**
 * Look up a license by Stripe checkout session ID.
 */
export async function getLicenseBySession(sessionId: string): Promise<License | null> {
  const licenses = await readLicenses();
  return licenses.find((l) => l.stripeSessionId === sessionId) || null;
}

/**
 * Look up all licenses for a given email address.
 */
export async function getLicenseByEmail(email: string): Promise<License[]> {
  const licenses = await readLicenses();
  return licenses.filter((l) => l.email.toLowerCase() === email.toLowerCase());
}
