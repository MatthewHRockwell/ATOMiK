import type { Metadata } from "next";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Privacy Policy - ATOMiK",
  description: "ATOMiK privacy policy. How we collect, use, and protect your data.",
};

export default function PrivacyPage() {
  return (
    <div className="min-h-screen bg-[#0a0a0f] text-[#e0e0e8]">
      <div className="max-w-3xl mx-auto px-6 py-16">
        <Link
          href="/"
          className="inline-block text-sm text-[#8888a0] hover:text-[#8b5cf6] transition-colors mb-12"
        >
          &larr; Back to atomik.tech
        </Link>

        <h1 className="text-4xl font-bold tracking-tight mb-2">Privacy Policy</h1>
        <p className="text-[#8888a0] text-sm mb-12">Effective March 1, 2026</p>

        <div className="space-y-10">
          <Section title="1. Introduction">
            <p>
              ATOMiK (&ldquo;we,&rdquo; &ldquo;us,&rdquo; or &ldquo;our&rdquo;) operates the
              atomik.tech website and provides delta-state computing software, including Python
              and C libraries and hardware IP cores. This policy describes what data we collect,
              how we use it, and your rights regarding that data.
            </p>
          </Section>

          <Section title="2. Information We Collect">
            <ul className="list-disc list-inside space-y-2 text-[#8888a0]">
              <li>
                <span className="text-[#e0e0e8]">Contact information</span> &mdash; name, email address, company, and workload details
                provided through contact, evaluation, or investor-diligence forms.
              </li>
              <li>
                <span className="text-[#e0e0e8]">Payment information</span> &mdash; processed
                by Stripe only when a paid evaluation or commercial engagement requires it. We do
                not store credit card numbers or bank details on our servers.
              </li>
              <li>
                <span className="text-[#e0e0e8]">Usage analytics</span> &mdash; anonymized
                telemetry such as page views, feature usage frequency, and error reports to improve
                the product.
              </li>
              <li>
                <span className="text-[#e0e0e8]">Communications</span> &mdash; emails, diligence requests,
                workload notes, or evaluation questions you send to us.
              </li>
            </ul>
          </Section>

          <Section title="3. How We Use Your Data">
            <ul className="list-disc list-inside space-y-2 text-[#8888a0]">
              <li>Respond to evaluation, diligence, licensing, and partnership inquiries.</li>
              <li>Process payments and billing communications where a paid engagement exists.</li>
              <li>Provide technical support or proof-review follow-up when requested.</li>
              <li>Improve our products, documentation, and user experience.</li>
              <li>Send product updates and security notices (you may opt out at any time).</li>
            </ul>
          </Section>

          <Section title="4. Third-Party Services">
            <p>We use the following third-party services that may process your data:</p>
            <ul className="list-disc list-inside space-y-2 text-[#8888a0] mt-3">
              <li>
                <span className="text-[#e0e0e8]">Stripe</span> &mdash; payment processing and optional lead/customer records.
                See{" "}
                <a
                  href="https://stripe.com/privacy"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-[#8b5cf6] hover:underline"
                >
                  Stripe&apos;s Privacy Policy
                </a>
                .
              </li>
              <li>
                <span className="text-[#e0e0e8]">Vercel</span> &mdash; website hosting and edge
                delivery. See{" "}
                <a
                  href="https://vercel.com/legal/privacy-policy"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-[#8b5cf6] hover:underline"
                >
                  Vercel&apos;s Privacy Policy
                </a>
                .
              </li>
            </ul>
          </Section>

          <Section title="5. Data Sharing">
            <p>
              We do not sell, rent, or trade your personal information to third parties. We share
              data only with the service providers listed above, solely for the purposes described
              in this policy.
            </p>
          </Section>

          <Section title="6. Data Retention">
            <p>
              We retain contact, inquiry, and evaluation records for as long as needed to respond,
              support diligence, or manage an active relationship. Payment records are retained as
              required by applicable tax and financial regulations. You may request deletion at any time.
            </p>
          </Section>

          <Section title="7. Your Rights">
            <p>You have the right to:</p>
            <ul className="list-disc list-inside space-y-2 text-[#8888a0] mt-3">
              <li>Access the personal data we hold about you.</li>
              <li>Request correction of inaccurate data.</li>
              <li>Request deletion of your data.</li>
              <li>Opt out of non-essential communications.</li>
            </ul>
            <p className="mt-3">
              To exercise any of these rights, contact us at{" "}
              <a
                href="mailto:mrockwell@atomik.tech"
                className="text-[#8b5cf6] hover:underline"
              >
                mrockwell@atomik.tech
              </a>
              .
            </p>
          </Section>

          <Section title="8. Security">
            <p>
              We implement industry-standard security measures to protect your data, including
              encrypted connections (TLS), secure payment processing via Stripe, and access controls
              on internal systems.
            </p>
          </Section>

          <Section title="9. Changes to This Policy">
            <p>
              We may update this policy from time to time. Material changes will be communicated
              via email or a notice on our website. Continued use of our services after changes
              constitutes acceptance.
            </p>
          </Section>

          <Section title="10. Contact">
            <p>
              For questions about this privacy policy or your data, contact us at{" "}
              <a
                href="mailto:mrockwell@atomik.tech"
                className="text-[#8b5cf6] hover:underline"
              >
                mrockwell@atomik.tech
              </a>
              .
            </p>
          </Section>
        </div>

        <div className="mt-16 pt-8 border-t border-[#1e1e2e] text-center text-sm text-[#8888a0]">
          &copy; 2026 ATOMiK. All rights reserved.
        </div>
      </div>
    </div>
  );
}

function Section({
  title,
  children,
}: {
  title: string;
  children: React.ReactNode;
}) {
  return (
    <section className="bg-[#12121a] border border-[#1e1e2e] rounded-xl p-8">
      <h2 className="text-xl font-semibold mb-4">{title}</h2>
      <div className="text-[#8888a0] leading-relaxed space-y-3">{children}</div>
    </section>
  );
}
