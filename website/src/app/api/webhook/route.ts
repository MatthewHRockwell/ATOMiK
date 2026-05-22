import { NextResponse } from 'next/server';

export async function POST() {
  return NextResponse.json({ received: true, disabled: 'self-serve licensing webhook disabled' });
}
