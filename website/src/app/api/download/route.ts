import { NextRequest, NextResponse } from 'next/server';
import { verifyDownloadToken } from '@/lib/license';

const GITHUB_TARBALL_URL =
  'https://github.com/MatthewHRockwell/ATOMiK/archive/refs/heads/main.tar.gz';

/**
 * GET /api/download?token=xxx
 * Verifies the download token and redirects to the GitHub source tarball.
 * The kernel module is Linux-only — no Windows/macOS support.
 */
export async function GET(req: NextRequest) {
  const token = req.nextUrl.searchParams.get('token');

  if (!token) {
    return NextResponse.json({ error: 'Missing download token' }, { status: 400 });
  }

  const verified = verifyDownloadToken(token);
  if (!verified) {
    return NextResponse.json(
      { error: 'Invalid or expired download token. Please return to your purchase confirmation to get a fresh link.' },
      { status: 403 }
    );
  }

  if (verified.platform !== 'linux') {
    return NextResponse.json(
      { error: 'ATOMiK kernel module is Linux-only. Please select the Linux platform.' },
      { status: 400 }
    );
  }

  return NextResponse.redirect(GITHUB_TARBALL_URL, 302);
}
