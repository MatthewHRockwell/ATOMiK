import { NextRequest, NextResponse } from 'next/server';
import { stripe, PRODUCTS } from '@/lib/stripe';

export async function POST(req: NextRequest) {
  try {
    const { tier, email } = await req.json();

    if (!tier || !PRODUCTS[tier as keyof typeof PRODUCTS]) {
      return NextResponse.json({ error: 'Invalid tier' }, { status: 400 });
    }

    const product = PRODUCTS[tier as keyof typeof PRODUCTS];
    const baseUrl = process.env.NEXT_PUBLIC_BASE_URL || 'http://localhost:3000';

    const session = await stripe.checkout.sessions.create({
      mode: product.mode,
      customer_email: email || undefined,
      line_items: [
        {
          price_data: {
            currency: 'usd',
            product_data: {
              name: product.name,
              description: product.description,
            },
            unit_amount: product.priceMonthly,
            recurring: { interval: 'month' },
          },
          quantity: 1,
        },
      ],
      metadata: {
        tier,
      },
      success_url: `${baseUrl}/success?session_id={CHECKOUT_SESSION_ID}`,
      cancel_url: `${baseUrl}/#pricing`,
    });

    return NextResponse.json({ url: session.url });
  } catch (error) {
    console.error('Checkout error:', error);
    return NextResponse.json(
      { error: 'Failed to create checkout session' },
      { status: 500 }
    );
  }
}
