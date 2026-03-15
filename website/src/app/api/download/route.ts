import { NextRequest, NextResponse } from 'next/server';
import { verifyDownloadToken } from '@/lib/license';

/**
 * GET /api/download?token=xxx
 * Serves the platform-specific installer after verifying the download token.
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

  const { platform } = verified;

  // TODO: Replace with actual signed URLs to package storage (S3, R2, etc.)
  // For now, return platform-specific install instructions
  const installInfo: Record<string, { filename: string; instructions: string }> = {
    linux: {
      filename: 'atomik-pro-0.1.0-linux-x86_64.tar.gz',
      instructions: 'tar xzf atomik-pro-*.tar.gz && cd atomik-pro && sudo ./install.sh',
    },
    windows: {
      filename: 'atomik-pro-0.1.0-windows-x64.exe',
      instructions: 'Run the installer and follow the prompts.',
    },
    macos: {
      filename: 'atomik-pro-0.1.0-macos-universal.pkg',
      instructions: 'Double-click the .pkg file and follow the prompts.',
    },
  };

  const info = installInfo[platform];
  if (!info) {
    return NextResponse.json({ error: 'Unknown platform' }, { status: 400 });
  }

  // When packages are built, this will redirect to the actual download:
  // return NextResponse.redirect(`https://releases.atomik.tech/${info.filename}`);

  return NextResponse.json({
    message: 'Download verified. Package builds are being finalized.',
    platform,
    filename: info.filename,
    instructions: info.instructions,
    status: 'coming_soon',
  });
}
