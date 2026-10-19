import { NextResponse } from 'next/server';
import type { NextRequest } from 'next/server';
import type Stripe from 'stripe';
import { stripe } from '@/lib/stripe';

export const runtime = 'nodejs';

function formatUsd(cents: number | null | undefined) {
  if (typeof cents !== 'number') return 'unknown amount';
  return new Intl.NumberFormat('en-US', {
    style: 'currency',
    currency: 'USD',
  }).format(cents / 100);
}

async function notifyPayment(session: Stripe.Checkout.Session) {
  const webhookUrl = process.env.LEAD_WEBHOOK_URL;
  if (!webhookUrl) return;

  const offer = session.metadata?.offer_name || session.metadata?.offer_id || 'ATOMiK paid evaluation';
  const email = session.customer_details?.email || session.customer_email || 'email not provided';
  const amount = formatUsd(session.amount_total);
  const stripeSession = session.id;
  const source = session.metadata?.source || '';
  const cta = session.metadata?.cta || '';
  const landingPath = session.metadata?.landing_path || '';
  const utmCampaign = session.metadata?.utm_campaign || '';

  const attributionLines = [
    source ? `Source: ${source}` : '',
    cta ? `CTA: ${cta}` : '',
    landingPath ? `Landing: ${landingPath}` : '',
    utmCampaign ? `Campaign: ${utmCampaign}` : '',
  ].filter(Boolean).join('\n');

  const payload = webhookUrl.includes('discord.com/api/webhooks')
    ? {
        content: `ATOMiK paid checkout completed: ${offer} (${amount})\nBuyer: ${email}\nStripe session: ${stripeSession}${attributionLines ? `\n${attributionLines}` : ''}`,
      }
    : {
        event: 'atomik_paid_checkout_completed',
        offer,
        amount,
        email,
        stripe_session: stripeSession,
        source,
        cta,
        landing_path: landingPath,
        utm_campaign: utmCampaign,
        paid_at: new Date().toISOString(),
      };

  await fetch(webhookUrl, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });
}

export async function POST(req: NextRequest) {
  const secret = process.env.STRIPE_WEBHOOK_SECRET;
  if (!secret) {
    return NextResponse.json({ error: 'Stripe webhook secret is not configured.' }, { status: 503 });
  }

  const signature = req.headers.get('stripe-signature');
  if (!signature) {
    return NextResponse.json({ error: 'Missing Stripe signature.' }, { status: 400 });
  }

  const payload = await req.text();
  let event: Stripe.Event;

  try {
    event = stripe.webhooks.constructEvent(payload, signature, secret);
  } catch (error) {
    console.error('[STRIPE_WEBHOOK] Signature verification failed:', error);
    return NextResponse.json({ error: 'Invalid Stripe webhook signature.' }, { status: 400 });
  }

  if (event.type === 'checkout.session.completed') {
    const session = event.data.object as Stripe.Checkout.Session;
    try {
      await notifyPayment(session);
    } catch (error) {
      console.error('[STRIPE_WEBHOOK] Payment notification failed:', error);
    }
  }

  return NextResponse.json({ received: true });
}
