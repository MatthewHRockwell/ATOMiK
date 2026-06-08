import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Payment Received - ATOMiK",
  description: "ATOMiK paid evaluation checkout confirmation.",
  robots: { index: false, follow: false },
};

const colors = {
  bg: "#070b12",
  panel: "#0d1420",
  border: "#1d324a",
  text: "#f4f8ff",
  muted: "#9fb1c7",
  cyan: "#22d3ee",
  blue: "#2563eb",
  green: "#53f28b",
};

export default function SuccessPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav />
      <main className="mx-auto max-w-3xl px-6 pb-24 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.green }}>
          Checkout complete
        </p>
        <div className="mt-5 rounded-lg p-8" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h1 className="text-3xl font-bold md:text-4xl">Payment received.</h1>
          <p className="mt-4 leading-7" style={{ color: colors.muted }}>
            Stripe has recorded the payment. ATOMiK will follow up to anchor the review on one workload, one constraint, and the evidence needed for the next decision.
          </p>
          <p className="mt-4 leading-7" style={{ color: colors.muted }}>
            If the buyer and ATOMiK continue into a larger scoped evaluation, the reservation is credited toward that written engagement.
          </p>
          <div className="mt-7 flex flex-col gap-3 sm:flex-row">
            <Link href="/contact?intent=evaluation" className="rounded-lg px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
              Add Workload Context
            </Link>
            <Link href="/investor-brief" className="rounded-lg px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
              View Investor Brief
            </Link>
          </div>
        </div>
      </main>
    </div>
  );
}
