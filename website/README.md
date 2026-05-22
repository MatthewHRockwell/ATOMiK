This is a [Next.js](https://nextjs.org) project bootstrapped with [`create-next-app`](https://nextjs.org/docs/app/api-reference/cli/create-next-app).

## Getting Started

First, run the development server:

```bash
npm run dev
# or
yarn dev
# or
pnpm dev
# or
bun dev
```

Open [http://localhost:3000](http://localhost:3000) with your browser to see the result.

You can start editing the page by modifying `app/page.tsx`. The page auto-updates as you edit the file.

This project uses [`next/font`](https://nextjs.org/docs/app/building-your-application/optimizing/fonts) to automatically optimize and load [Geist](https://vercel.com/font), a new font family for Vercel.

## Learn More

To learn more about Next.js, take a look at the following resources:

- [Next.js Documentation](https://nextjs.org/docs) - learn about Next.js features and API.
- [Learn Next.js](https://nextjs.org/learn) - an interactive Next.js tutorial.

You can check out [the Next.js GitHub repository](https://github.com/vercel/next.js) - your feedback and contributions are welcome!

## Deploy on Vercel

The easiest way to deploy your Next.js app is to use the [Vercel Platform](https://vercel.com/new?utm_medium=default-template&filter=next.js&utm_source=create-next-app&utm_campaign=create-next-app-readme) from the creators of Next.js.

Check out our [Next.js deployment documentation](https://nextjs.org/docs/app/building-your-application/deploying) for more details.

## Lead Alerts

The contact and evaluation forms post to `/api/subscribe`. Production stores leads in Stripe customer metadata when `STRIPE_SECRET_KEY` is configured. For immediate human notification, set `LEAD_WEBHOOK_URL` in Vercel Production.

Supported webhook targets:

- Slack incoming webhook: inferred from `hooks.slack.com`, or set `LEAD_WEBHOOK_FORMAT=slack`.
- Discord webhook: inferred from `discord.com`, or set `LEAD_WEBHOOK_FORMAT=discord`.
- Make, Zapier, CRM, or custom endpoint: use `LEAD_WEBHOOK_FORMAT=generic` or omit it.

Useful commands:

```bash
vercel env add LEAD_WEBHOOK_URL Production
vercel env add LEAD_WEBHOOK_FORMAT Production
vercel deploy --prod --yes
```

If no webhook is configured, submissions still persist to Stripe and emit `[LEAD_ALERT]` plus `[LEAD_ALERT_NO_WEBHOOK]` in Vercel function logs.
