import { NextResponse } from 'next/server';

export async function POST() {
  return NextResponse.json(
    {
      error: 'Self-serve checkout is disabled. Request evaluation access through /contact.',
    },
    { status: 410 }
  );
}
