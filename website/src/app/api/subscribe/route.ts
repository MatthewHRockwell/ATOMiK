import { NextRequest, NextResponse } from 'next/server';

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

type LeadMetadata = Record<string, string>;
type LeadWebhookFormat = 'generic' | 'slack' | 'discord';

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

function inferWebhookFormat(webhookUrl: string): LeadWebhookFormat {
  const configured = process.env.LEAD_WEBHOOK_FORMAT?.toLowerCase();
  if (configured === 'slack' || configured === 'discord' || configured === 'generic') {
    return configured;
  }

  try {
    const host = new URL(webhookUrl).host;
    if (host.includes('slack.com')) return 'slack';
    if (host.includes('discord.com')) return 'discord';
  } catch {
    return 'generic';
  }

  return 'generic';
}

function compactLead(email: string, metadata: LeadMetadata) {
  return {
    email,
    source: metadata.source,
    name: metadata.name,
    company: metadata.company,
    role: metadata.role,
    requested_path: metadata.requested_path,
    pain_category: metadata.pain_category,
    timeline: metadata.timeline,
    use_case: metadata.use_case,
    current_stack: metadata.current_stack,
    interests: metadata.interests,
    message: metadata.message,
    submitted_at: metadata.subscribed_at,
  };
}

function leadText(email: string, metadata: LeadMetadata) {
  const lines = [
    'New ATOMiK lead',
    `Email: ${email}`,
    metadata.name ? `Name: ${metadata.name}` : '',
    metadata.company ? `Company: ${metadata.company}` : '',
    metadata.role ? `Role: ${metadata.role}` : '',
    metadata.source ? `Source: ${metadata.source}` : '',
    metadata.requested_path ? `Intent: ${metadata.requested_path}` : '',
    metadata.pain_category ? `Pain: ${metadata.pain_category}` : '',
    metadata.timeline ? `Timeline: ${metadata.timeline}` : '',
    metadata.use_case ? `Use case: ${metadata.use_case}` : '',
    metadata.current_stack ? `Stack: ${metadata.current_stack}` : '',
    metadata.interests ? `Interests: ${metadata.interests}` : '',
    metadata.message ? `Message: ${metadata.message}` : '',
  ];

  return lines.filter(Boolean).join('\n');
}

function slackPayload(email: string, metadata: LeadMetadata) {
  const fields = [
    ['Email', email],
    ['Name', metadata.name],
    ['Company', metadata.company],
    ['Role', metadata.role],
    ['Source', metadata.source],
    ['Intent', metadata.requested_path],
    ['Pain', metadata.pain_category],
    ['Timeline', metadata.timeline],
  ]
    .filter(([, value]) => value)
    .map(([label, value]) => ({ type: 'mrkdwn', text: `*${label}:*\n${value}` }));

  return {
    text: `New ATOMiK lead: ${metadata.company || metadata.name || email}`,
    blocks: [
      {
        type: 'section',
        text: { type: 'mrkdwn', text: '*New ATOMiK lead*' },
      },
      ...(fields.length ? [{ type: 'section', fields }] : []),
      ...(metadata.message
        ? [{ type: 'section', text: { type: 'mrkdwn', text: `*Message:*\n${metadata.message}` } }]
        : []),
    ],
  };
}

function discordPayload(email: string, metadata: LeadMetadata) {
  const fields = [
    ['Email', email],
    ['Name', metadata.name],
    ['Company', metadata.company],
    ['Role', metadata.role],
    ['Source', metadata.source],
    ['Intent', metadata.requested_path],
    ['Pain', metadata.pain_category],
    ['Timeline', metadata.timeline],
    ['Use case', metadata.use_case],
    ['Message', metadata.message],
  ]
    .filter(([, value]) => value)
    .map(([name, value]) => ({ name, value: String(value).slice(0, 1024), inline: name !== 'Message' && name !== 'Use case' }));

  return {
    content: `New ATOMiK lead: ${metadata.company || metadata.name || email}`,
    embeds: [
      {
        title: 'New ATOMiK lead',
        color: 0x4f8fff,
        fields,
        timestamp: metadata.subscribed_at,
      },
    ],
  };
}

function buildWebhookPayload(email: string, metadata: LeadMetadata, format: LeadWebhookFormat) {
  if (format === 'slack') return slackPayload(email, metadata);
  if (format === 'discord') return discordPayload(email, metadata);
  return {
    event: 'atomik.lead.created',
    text: leadText(email, metadata),
    lead: compactLead(email, metadata),
  };
}

function logLeadAlert(email: string, metadata: LeadMetadata, deliveryPaths: string[]) {
  console.warn('[LEAD_ALERT] ' + JSON.stringify({
    ...compactLead(email, metadata),
    delivery_paths: deliveryPaths,
    stripe_customer_record: deliveryPaths.includes('stripe'),
    webhook_delivered: deliveryPaths.includes('webhook'),
  }));
}

async function notifyLead(email: string, metadata: LeadMetadata) {
  const webhookUrl = process.env.LEAD_WEBHOOK_URL;
  if (!webhookUrl) return false;

  const format = inferWebhookFormat(webhookUrl);
  const res = await fetch(webhookUrl, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(buildWebhookPayload(email, metadata, format)),
  });

  if (!res.ok) {
    throw new Error('Lead webhook failed with status ' + res.status + ' using ' + format + ' format');
  }

  return true;
}

/**
 * POST /api/subscribe
 * Captures newsletter, evaluation, demo, licensing, and design-partner leads.
 *
 * Stripe customer metadata is the durable store. LEAD_WEBHOOK_URL mirrors each
 * submission into Slack, Discord, Make, Zapier, CRM, or another lightweight inbox.
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
    const deliveryPaths: string[] = [];
    let stored = false;

    try {
      if (await persistToStripe(normalized, metadata)) {
        stored = true;
        deliveryPaths.push('stripe');
      }
    } catch (stripeError) {
      errors.push('stripe');
      console.error('[SUBSCRIBE] Stripe persistence failed:', stripeError);
    }

    try {
      if (await notifyLead(normalized, metadata)) {
        stored = true;
        deliveryPaths.push('webhook');
      }
    } catch (webhookError) {
      errors.push('webhook');
      console.error('[SUBSCRIBE] Lead notification failed:', webhookError);
    }

    if (stored) {
      logLeadAlert(normalized, metadata, deliveryPaths);
    }

    if (stored && !process.env.LEAD_WEBHOOK_URL) {
      console.warn('[LEAD_ALERT_NO_WEBHOOK] Lead stored in Stripe but no LEAD_WEBHOOK_URL is configured. Add a Slack, Discord, Make, Zapier, or CRM webhook for immediate human notification.');
    }

    if (!stored) {
      console.error('[SUBSCRIBE] Lead not captured: ' + normalized, { source: metadata.source, path: metadata.requested_path, errors });
      return NextResponse.json(
        { error: 'Lead capture is temporarily unavailable. Please email mrockwell@atomik.tech directly.' },
        { status: 503 }
      );
    }

    console.log('[SUBSCRIBE] captured ' + normalized + ' (source: ' + metadata.source + '; path: ' + (metadata.requested_path || 'n/a') + '; delivery: ' + deliveryPaths.join(',') + ')');

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Subscribe error:', error);
    return NextResponse.json(
      { error: 'Failed to subscribe' },
      { status: 500 }
    );
  }
}
