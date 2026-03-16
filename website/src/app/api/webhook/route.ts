import { NextRequest, NextResponse } from 'next/server';
import { stripe } from '@/lib/stripe';
import { generateSecureLicenseKey, saveLicense } from '@/lib/licenses';

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

      // Persist the license to local JSON store (idempotent on retry)
      const license = await saveLicense({
        key: licenseKey,
        email,
        tier,
        stripeSessionId: session.id,
        createdAt: new Date().toISOString(),
      });

      console.log(`[LICENSE] Generated for ${email}: ${license.key} (${tier})`);
      console.log(`[LICENSE] Session: ${session.id}, Subscription: ${subscriptionId}`);

      // Update the Stripe subscription metadata with the license key
      if (subscriptionId) {
        try {
          await stripe.subscriptions.update(subscriptionId, {
            metadata: { license_key: license.key, tier },
          });
        } catch (err) {
          console.error(`[LICENSE] Failed to update subscription metadata:`, err);
          // Non-fatal: license is already persisted locally
        }
      }

      // TODO: Send license email via SendGrid/Resend
      // await sendLicenseEmail({ email, licenseKey: license.key, tier });

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
