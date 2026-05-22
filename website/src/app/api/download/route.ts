import { NextResponse } from 'next/server';

export async function GET() {
  return NextResponse.json(
    {
      error: 'Self-serve downloads are disabled. Public artifacts are available through GitHub and the proof pages.',
    },
    { status: 410 }
  );
}
