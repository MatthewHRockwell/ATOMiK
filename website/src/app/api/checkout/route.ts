import { NextResponse } from 'next/server';
import type { NextRequest } from 'next/server';
import { getPaidOffer } from '@/lib/offers';
import { stripe } from '@/lib/stripe';

function checkoutOrigin(req: NextRequest) {
  const configured = process.env.NEXT_PUBLIC_BASE_URL?.trim().replace(/\/$/, '');
  if (!configured) return req.nextUrl.origin;
  if (configured.startsWith('http://') || configured.startsWith('https://')) return configured;
  return `https://${configured}`;
}

function clip(value: unknown, max = 450) {
  if (typeof value !== 'string') return '';
  return value.trim().slice(0, max);
}

function compactMetadata(values: Record<string, string>) {
  return Object.fromEntries(Object.entries(values).filter(([, value]) => value));
}

function checkoutMetadata(offer: { id: string; name: string }, body: Record<string, unknown>) {
  return compactMetadata({
    offer_id: offer.id,
    offer_name: offer.name,
    source: clip(body.source) || 'pricing',
    cta: clip(body.cta),
    landing_path: clip(body.landing_path),
    referrer: clip(body.referrer),
    utm_source: clip(body.utm_source),
    utm_medium: clip(body.utm_medium),
    utm_campaign: clip(body.utm_campaign),
    utm_content: clip(body.utm_content),
    utm_term: clip(body.utm_term),
  });
}

export async function POST(req: NextRequest) {
  const body = await req.json().catch(() => ({}));
  const offer = getPaidOffer(body.offerId);

  if (!offer) {
    return NextResponse.json(
      { error: 'Unknown paid evaluation offer.' },
      { status: 400 }
    );
  }

  if (!process.env.STRIPE_SECRET_KEY) {
    return NextResponse.json(
      { error: 'Stripe checkout is not configured.' },
      { status: 503 }
    );
  }

  try {
    const origin = checkoutOrigin(req);
    const metadata = checkoutMetadata(offer, body);
    const session = await stripe.checkout.sessions.create({
      mode: 'payment',
      customer_creation: 'always',
      line_items: [
        {
          quantity: 1,
          price_data: {
            currency: 'usd',
            unit_amount: offer.priceCents,
            product_data: {
              name: offer.name,
              description: offer.checkoutDescription,
              metadata: {
                offer_id: offer.id,
              },
            },
          },
        },
      ],
      metadata,
      payment_intent_data: {
        metadata,
      },
      allow_promotion_codes: true,
      billing_address_collection: 'auto',
      success_url: `${origin}/success?offer=${offer.id}&session_id={CHECKOUT_SESSION_ID}`,
      cancel_url: `${origin}/pricing?checkout=cancelled&offer=${offer.id}`,
    });

    return NextResponse.json({ url: session.url });
  } catch (error) {
    console.error('[CHECKOUT] Failed to create Stripe Checkout session:', error);
    return NextResponse.json(
      { error: 'Failed to open checkout.' },
      { status: 502 }
    );
  }
}
