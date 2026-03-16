import { NextRequest, NextResponse } from 'next/server';
import { stripe } from '@/lib/stripe';
import { generateSecureLicenseKey } from '@/lib/licenses';

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

      // Generate a cryptographically random license key
      const licenseKey = generateSecureLicenseKey();

      console.log(`[LICENSE] Generated for ${email}: ${licenseKey} (${tier})`);
      console.log(`[LICENSE] Session: ${session.id}, Subscription: ${subscriptionId}`);

      // Store the license key in Stripe subscription metadata (source of truth)
      if (subscriptionId) {
        try {
          await stripe.subscriptions.update(subscriptionId, {
            metadata: { license_key: licenseKey, tier },
          });
          console.log(`[LICENSE] Stored in subscription ${subscriptionId} metadata`);
        } catch (err) {
          console.error(`[LICENSE] Failed to update subscription metadata:`, err);
        }
      } else {
        console.warn(`[LICENSE] No subscription ID — license key not persisted!`);
      }

      // TODO: Send license email via SendGrid/Resend
      // await sendLicenseEmail({ email, licenseKey, tier });

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
