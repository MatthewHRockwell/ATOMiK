import Stripe from 'stripe';

let _stripe: Stripe | null = null;

function getStripeClient(): Stripe {
  if (!_stripe) {
    const key = process.env.STRIPE_SECRET_KEY;
    if (!key) {
      throw new Error('STRIPE_SECRET_KEY is not set');
    }
    _stripe = new Stripe(key, {
      apiVersion: '2026-02-25.clover',
      typescript: true,
    });
  }
  return _stripe;
}

// Lazy initialization — only throws when actually used at runtime, not at build time
export const stripe = new Proxy({} as Stripe, {
  get(_, prop) {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    return (getStripeClient() as any)[prop];
  },
});

// Product/price configuration
export const PRODUCTS = {
  professional: {
    name: 'ATOMiK Pro',
    description: 'Linux kernel module + system-level optimization + priority support',
    mode: 'subscription' as const,
    priceMonthly: 9900, // $99.00 in cents
  },
  team: {
    name: 'ATOMiK Team',
    description: 'SDK generation pipeline (5 languages) + waste analysis + team license',
    mode: 'subscription' as const,
    priceMonthly: 29900, // $299.00 in cents
  },
  enterprise: {
    name: 'ATOMiK Enterprise',
    description: 'FPGA hardware acceleration + custom CPU + dedicated support + consulting',
    mode: 'subscription' as const,
    priceMonthly: 99900, // $999.00 in cents
  },
} as const;
