import { NextResponse } from 'next/server';

export async function GET() {
  return NextResponse.json(
    {
      error: 'Legacy API downloads are disabled. Public proof digest files are available from /benchmarks and /proof-artifacts/.',
    },
    { status: 410 }
  );
}
