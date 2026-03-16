import { NextRequest, NextResponse } from 'next/server';

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

/**
 * POST /api/subscribe
 * Captures email addresses for the newsletter/mailing list.
 *
 * Storage strategy: create a Stripe customer (if Stripe is configured),
 * with metadata marking them as a newsletter subscriber. This survives
 * Vercel redeployments because Stripe is the source of truth.
 * Falls back to console.log if Stripe is not configured (dev mode).
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

    // Try Stripe customer creation for persistent storage
    const stripeKey = process.env.STRIPE_SECRET_KEY;
    if (stripeKey) {
      try {
        const { stripe } = await import('@/lib/stripe');

        // Check if customer already exists
        const existing = await stripe.customers.list({ email: normalized, limit: 1 });
        if (existing.data.length === 0) {
          await stripe.customers.create({
            email: normalized,
            metadata: {
              source: body.source || 'newsletter',
              name: body.name || '',
              subscribed_at: new Date().toISOString(),
            },
          });
        }
      } catch (stripeError) {
        // Stripe failure is non-fatal — still return success to the user
        console.error('[SUBSCRIBE] Stripe error (non-fatal):', stripeError);
      }
    }

    // Always log for debugging/backup
    console.log(`[SUBSCRIBE] ${normalized} (source: ${body.source || 'newsletter'})`);

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Subscribe error:', error);
    return NextResponse.json(
      { error: 'Failed to subscribe' },
      { status: 500 }
    );
  }
}
