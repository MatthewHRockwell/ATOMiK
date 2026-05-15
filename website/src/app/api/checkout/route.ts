import { NextResponse } from 'next/server';

export async function POST() {
  return NextResponse.json(
    {
      error:
        "Self-serve checkout is not the public evaluation model. Request technical evaluation or licensing access through the contact form.",
      contact: "/contact?intent=evaluation",
    },
    { status: 410 }
  );
}
