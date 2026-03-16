import { NextRequest, NextResponse } from 'next/server';
import { stripe } from '@/lib/stripe';
import { generateDownloadToken } from '@/lib/license';

/**
 * GET /api/license?session_id=cs_xxx
 * Returns license key and download links after successful checkout.
 *
 * Lookup: session_id -> Stripe session -> subscription_id -> subscription metadata.
 * Stripe subscription metadata is the sole source of truth for license keys.
 */
export async function GET(req: NextRequest) {
  const sessionId = req.nextUrl.searchParams.get('session_id');

  if (!sessionId) {
    return NextResponse.json({ error: 'Missing session_id' }, { status: 400 });
  }

  try {
    const session = await stripe.checkout.sessions.retrieve(sessionId);

    if (session.payment_status !== 'paid') {
      return NextResponse.json({ error: 'Payment not completed' }, { status: 402 });
    }

    const email = session.customer_email || session.customer_details?.email || '';
    const tier = (session.metadata?.tier || 'professional') as 'professional' | 'enterprise';

    // Look up license key from Stripe subscription metadata
    let licenseKey: string | null = null;

    const subscriptionId = typeof session.subscription === 'string'
      ? session.subscription
      : session.subscription?.id || '';

    if (subscriptionId) {
      try {
        const subscription = await stripe.subscriptions.retrieve(subscriptionId);
        if (subscription.metadata?.license_key) {
          licenseKey = subscription.metadata.license_key;
        }
      } catch {
        // Subscription lookup failed -- continue without it
      }
    }

    if (!licenseKey) {
      return NextResponse.json(
        { error: 'License not yet generated. Please wait a moment and refresh.' },
        { status: 404 }
      );
    }

    // Generate signed download tokens for each platform
    const downloads = {
      linux: generateDownloadToken(licenseKey, 'linux'),
      windows: generateDownloadToken(licenseKey, 'windows'),
      macos: generateDownloadToken(licenseKey, 'macos'),
    };

    const baseUrl = process.env.NEXT_PUBLIC_BASE_URL || 'http://localhost:3000';

    return NextResponse.json({
      licenseKey,
      tier,
      email,
      downloads: {
        linux: `${baseUrl}/api/download?token=${downloads.linux}`,
        windows: `${baseUrl}/api/download?token=${downloads.windows}`,
        macos: `${baseUrl}/api/download?token=${downloads.macos}`,
      },
      install: {
        linux: `git clone https://github.com/MatthewHRockwell/ATOMiK.git && cd ATOMiK/software/atomik_kmod && sudo ./install.sh --license ${licenseKey}`,
        pip: `pip install atomik-core`,
      },
    });
  } catch (error) {
    console.error('License retrieval error:', error);
    return NextResponse.json({ error: 'Failed to retrieve license' }, { status: 500 });
  }
}
