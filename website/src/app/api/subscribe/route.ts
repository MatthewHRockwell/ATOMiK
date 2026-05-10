import { NextRequest, NextResponse } from 'next/server';

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

function clip(value: unknown, max = 450) {
  if (typeof value !== 'string') return '';
  return value.trim().slice(0, max);
}

/**
 * POST /api/subscribe
 * Captures newsletter, evaluation, demo, and design-partner leads.
 *
 * Storage strategy: create a Stripe customer when Stripe is configured, with
 * metadata marking the source and requested path. This avoids inventing a CRM
 * backend while preserving useful lead context.
 */
export async function POST(req: NextRequest) {
  try {
    const body = await req.json();
    const email = body?.email;

    if (!email || typeof email !== 'string') {
      return NextResponse.json({ error: 'Email is required' }, { status: 400 });
    }

    const normalized = email.trim().toLowerCase();

    if (!EMAIL_RE.test(normalized)) {
      return NextResponse.json({ error: 'Invalid email format' }, { status: 400 });
    }

    const metadata = {
      source: clip(body.source) || 'newsletter',
      name: clip(body.name),
      role: clip(body.role),
      company: clip(body.company),
      requested_path: clip(body.requested_path || body.plan),
      pain_category: clip(body.pain_category),
      timeline: clip(body.timeline),
      use_case: clip(body.use_case),
      current_stack: clip(body.current_stack),
      message: clip(body.message),
      interests: Array.isArray(body.interests) ? body.interests.map((item: unknown) => clip(item, 80)).filter(Boolean).join(', ') : '',
      subscribed_at: new Date().toISOString(),
    };

    const stripeKey = process.env.STRIPE_SECRET_KEY;
    if (stripeKey) {
      try {
        const { stripe } = await import('@/lib/stripe');

        const existing = await stripe.customers.list({ email: normalized, limit: 1 });
        if (existing.data.length === 0) {
          await stripe.customers.create({
            email: normalized,
            name: metadata.name || undefined,
            metadata,
          });
        }
      } catch (stripeError) {
        console.error('[SUBSCRIBE] Stripe error (non-fatal):', stripeError);
      }
    }

    console.log(`[SUBSCRIBE] ${normalized} (source: ${metadata.source}; path: ${metadata.requested_path || 'n/a'})`);

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Subscribe error:', error);
    return NextResponse.json(
      { error: 'Failed to subscribe' },
      { status: 500 }
    );
  }
}
