import { NextRequest, NextResponse } from 'next/server';
import { stripe } from '@/lib/stripe';
import { generateLicenseKey } from '@/lib/license';

export async function POST(req: NextRequest) {
  const body = await req.text();
  const sig = req.headers.get('stripe-signature');

  if (!sig || !process.env.STRIPE_WEBHOOK_SECRET) {
    return NextResponse.json({ error: 'Missing signature' }, { status: 400 });
  }

  let event;
  try {
    event = stripe.webhooks.constructEvent(body, sig, process.env.STRIPE_WEBHOOK_SECRET);
  } catch (err) {
    console.error('Webhook signature verification failed:', err);
    return NextResponse.json({ error: 'Invalid signature' }, { status: 400 });
  }

  switch (event.type) {
    case 'checkout.session.completed': {
      const session = event.data.object;
      const email = session.customer_email || session.customer_details?.email || '';
      const tier = session.metadata?.tier || 'professional';
      const subscriptionId = typeof session.subscription === 'string'
        ? session.subscription
        : session.subscription?.id || '';

      const licenseKey = generateLicenseKey(email, tier, subscriptionId);

      console.log(`[LICENSE] Generated for ${email}: ${licenseKey} (${tier})`);

      // TODO: Store in database and send email via SendGrid/Resend
      // For now, the license key is available on the success page via session metadata

      // Update the subscription metadata with the license key
      if (subscriptionId) {
        await stripe.subscriptions.update(subscriptionId, {
          metadata: { license_key: licenseKey, tier },
        });
      }

      break;
    }

    case 'customer.subscription.deleted': {
      const subscription = event.data.object;
      console.log(`[LICENSE] Subscription cancelled: ${subscription.id}`);
      // TODO: Revoke license key
      break;
    }
  }

  return NextResponse.json({ received: true });
}
