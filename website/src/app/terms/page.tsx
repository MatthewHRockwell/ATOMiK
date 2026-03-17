import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Terms of Service - ATOMiK",
  description: "ATOMiK terms of service. License terms, billing, and usage policies.",
};

export default function TermsPage() {
  return (
    <div className="min-h-screen bg-[#0a0a0f] text-[#e0e0e8]">
      <div className="max-w-3xl mx-auto px-6 py-16">
        <a
          href="/"
          className="inline-block text-sm text-[#8888a0] hover:text-[#8b5cf6] transition-colors mb-12"
        >
          &larr; Back to atomik.tech
        </a>

        <h1 className="text-4xl font-bold tracking-tight mb-2">Terms of Service</h1>
        <p className="text-[#8888a0] text-sm mb-12">Effective March 1, 2026</p>

        <div className="space-y-10">
          <Section title="1. Acceptance of Terms">
            <p>
              By accessing or using ATOMiK software, hardware IP, or services provided through
              atomik.tech (&ldquo;Services&rdquo;), you agree to be bound by these Terms of
              Service. If you do not agree, do not use the Services.
            </p>
          </Section>

          <Section title="2. License Tiers">
            <p>ATOMiK is available under three license tiers:</p>
            <div className="mt-4 space-y-4">
              <TierCard
                name="Community"
                price="Free"
                description="Open-source license for personal and non-commercial use. Includes the ATOMiK Python library and C library under the terms of the applicable open-source license."
              />
              <TierCard
                name="Professional"
                price="$99/month"
                description="Commercial license for individuals and small teams. Includes priority support (48-hour SLA), commercial use rights, and access to pre-built binaries."
              />
              <TierCard
                name="Team"
                price="$299/month"
                description="SDK generation pipeline (5 languages), waste analysis tools, team license (5 seats), and CI integration."
              />
              <TierCard
                name="Enterprise"
                price="$999/month"
                description="Full commercial license for organizations. Includes hardware IP cores, 4-hour support SLA, custom integration assistance, and dedicated account management."
              />
            </div>
          </Section>

          <Section title="3. Subscription and Billing">
            <ul className="list-disc list-inside space-y-2">
              <li>
                Paid subscriptions (Professional and Enterprise) are billed monthly via Stripe.
              </li>
              <li>
                Your subscription renews automatically on the same day each month unless cancelled.
              </li>
              <li>
                You may cancel at any time. Cancellation takes effect at the end of the current
                billing period. No partial refunds are provided for the remaining days in a billing
                cycle.
              </li>
              <li>
                We reserve the right to change pricing with 30 days&apos; written notice. Price
                changes take effect at the start of your next billing cycle after the notice period.
              </li>
              <li>
                Failed payments will be retried per Stripe&apos;s retry schedule. Access may be
                suspended after repeated payment failures.
              </li>
            </ul>
          </Section>

          <Section title="4. Intellectual Property">
            <p>
              All ATOMiK software, hardware designs, documentation, trademarks, and related
              intellectual property are owned by ATOMiK. Your license grants you the right to use
              the software and hardware IP according to your tier&apos;s terms. You do not acquire
              any ownership rights.
            </p>
            <ul className="list-disc list-inside space-y-2 mt-3">
              <li>
                You may not reverse-engineer, decompile, or disassemble proprietary components of
                the software beyond what is permitted by applicable law.
              </li>
              <li>
                You may not sublicense, resell, or redistribute the software except as expressly
                permitted by your license tier.
              </li>
              <li>
                Open-source components included in the Community tier are governed by their
                respective open-source licenses.
              </li>
            </ul>
          </Section>

          <Section title="5. Acceptable Use">
            <p>You agree not to:</p>
            <ul className="list-disc list-inside space-y-2 mt-3">
              <li>Use the Services for any unlawful purpose.</li>
              <li>
                Attempt to gain unauthorized access to our systems or other users&apos; accounts.
              </li>
              <li>Interfere with or disrupt the Services or their infrastructure.</li>
              <li>
                Use Community-tier software for commercial purposes without upgrading to a paid
                license.
              </li>
            </ul>
          </Section>

          <Section title="6. Limitation of Liability">
            <p>
              To the maximum extent permitted by applicable law, ATOMiK and its officers,
              directors, and employees shall not be liable for any indirect, incidental, special,
              consequential, or punitive damages, including but not limited to loss of profits,
              data, or business opportunities, arising from your use of the Services.
            </p>
            <p>
              Our total aggregate liability for any claims arising under these Terms shall not
              exceed the amount you paid to ATOMiK in the twelve (12) months preceding the claim.
            </p>
          </Section>

          <Section title="7. Disclaimer of Warranties">
            <p>
              The Services are provided &ldquo;as is&rdquo; and &ldquo;as available&rdquo; without
              warranties of any kind, whether express or implied, including but not limited to
              implied warranties of merchantability, fitness for a particular purpose, and
              non-infringement.
            </p>
          </Section>

          <Section title="8. Termination">
            <ul className="list-disc list-inside space-y-2">
              <li>
                You may terminate your account at any time by cancelling your subscription and
                contacting us to request account deletion.
              </li>
              <li>
                We may suspend or terminate your access if you violate these Terms, with or without
                notice depending on the severity of the violation.
              </li>
              <li>
                Upon termination, your license to use paid features ends immediately. You retain
                the right to use Community-tier features under the applicable open-source license.
              </li>
              <li>
                Sections 4, 6, 7, and 9 survive termination.
              </li>
            </ul>
          </Section>

          <Section title="9. Governing Law">
            <p>
              These Terms are governed by and construed in accordance with the laws of the State of
              California, United States, without regard to conflict of law principles.
            </p>
          </Section>

          <Section title="10. Changes to These Terms">
            <p>
              We may update these Terms from time to time. Material changes will be communicated
              via email or a notice on our website at least 30 days before they take effect.
              Continued use of the Services after changes constitutes acceptance.
            </p>
          </Section>

          <Section title="11. Contact">
            <p>
              For questions about these Terms of Service, contact us at{" "}
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

function TierCard({
  name,
  price,
  description,
}: {
  name: string;
  price: string;
  description: string;
}) {
  return (
    <div className="bg-[#0a0a0f] border border-[#1e1e2e] rounded-lg p-5">
      <div className="flex items-baseline justify-between mb-2">
        <h3 className="text-[#e0e0e8] font-semibold">{name}</h3>
        <span className="text-[#8b5cf6] font-mono text-sm font-semibold">{price}</span>
      </div>
      <p className="text-sm">{description}</p>
    </div>
  );
}
