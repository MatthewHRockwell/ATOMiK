import { NextResponse } from 'next/server';

export async function GET() {
  return NextResponse.json(
    {
      error: 'Self-serve license retrieval is disabled. Request evaluation access through /contact.',
    },
    { status: 410 }
  );
}
