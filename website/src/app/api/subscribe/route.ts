import { NextRequest, NextResponse } from 'next/server';

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

type LeadMetadata = Record<string, string>;

function clip(value: unknown, max = 450) {
  if (typeof value !== 'string') return '';
  return value.trim().slice(0, max);
}

function buildMetadata(body: Record<string, unknown>): LeadMetadata {
  return {
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
}

async function persistToStripe(email: string, metadata: LeadMetadata) {
  if (!process.env.STRIPE_SECRET_KEY) return false;

  const { stripe } = await import('@/lib/stripe');
  const existing = await stripe.customers.list({ email, limit: 1 });

  if (existing.data.length === 0) {
    await stripe.customers.create({
      email,
      name: metadata.name || undefined,
      metadata: {
        ...metadata,
        lead_submissions: '1',
      },
    });
    return true;
  }

  const customer = existing.data[0];
  const priorCount = Number.parseInt(customer.metadata?.lead_submissions || '0', 10);

  await stripe.customers.update(customer.id, {
    name: metadata.name || customer.name || undefined,
    metadata: {
      ...customer.metadata,
      ...metadata,
      lead_submissions: String(Number.isFinite(priorCount) ? priorCount + 1 : 1),
      latest_source: metadata.source,
      latest_requested_path: metadata.requested_path,
      latest_pain_category: metadata.pain_category,
      latest_timeline: metadata.timeline,
      latest_submission_at: metadata.subscribed_at,
    },
  });

  return true;
}

async function notifyLead(email: string, metadata: LeadMetadata) {
  const webhookUrl = process.env.LEAD_WEBHOOK_URL;
  if (!webhookUrl) return false;

  const res = await fetch(webhookUrl, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ email, ...metadata }),
  });

  if (!res.ok) {
    throw new Error('Lead webhook failed with status ' + res.status);
  }

  return true;
}

/**
 * POST /api/subscribe
 * Captures newsletter, evaluation, demo, licensing, and design-partner leads.
 *
 * A public form must not silently succeed unless the lead was durably stored or
 * delivered. Stripe customer metadata is the default store; LEAD_WEBHOOK_URL can
 * mirror each submission into email, Slack, CRM, or another lightweight inbox.
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

    const metadata = buildMetadata(body);
    const errors: string[] = [];
    let stored = false;

    try {
      stored = await persistToStripe(normalized, metadata) || stored;
    } catch (stripeError) {
      errors.push('stripe');
      console.error('[SUBSCRIBE] Stripe persistence failed:', stripeError);
    }

    try {
      stored = await notifyLead(normalized, metadata) || stored;
    } catch (webhookError) {
      errors.push('webhook');
      console.error('[SUBSCRIBE] Lead notification failed:', webhookError);
    }

    if (!stored) {
      console.error('[SUBSCRIBE] Lead not captured: ' + normalized, { source: metadata.source, path: metadata.requested_path, errors });
      return NextResponse.json(
        { error: 'Lead capture is temporarily unavailable. Please email mrockwell@atomik.tech directly.' },
        { status: 503 }
      );
    }

    console.log('[SUBSCRIBE] captured ' + normalized + ' (source: ' + metadata.source + '; path: ' + (metadata.requested_path || 'n/a') + ')');

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Subscribe error:', error);
    return NextResponse.json(
      { error: 'Failed to subscribe' },
      { status: 500 }
    );
  }
}
