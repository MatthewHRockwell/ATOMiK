import type { Metadata } from "next";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Terms of Service - ATOMiK",
  description: "ATOMiK terms of service. License terms, billing, and usage policies.",
};

export default function TermsPage() {
  return (
    <div className="min-h-screen bg-[#0a0a0f] text-[#e0e0e8]">
      <div className="max-w-3xl mx-auto px-6 py-16">
        <Link
          href="/"
          className="inline-block text-sm text-[#8888a0] hover:text-[#8b5cf6] transition-colors mb-12"
        >
          &larr; Back to atomik.tech
        </Link>

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

          <Section title="2. Evaluation and Licensing">
            <p>
              ATOMiK public evaluation is request-based. Commercial licensing,
              support, and hardware/IP terms require a written agreement.
            </p>
            <div className="mt-4 space-y-4">
              <TierCard
                name="Public Repository"
                price="Public review"
                description="Public source, documentation, proof notes, and artifacts are available for technical review subject to the license terms in each package or directory."
              />
              <TierCard
                name="Evaluation Access"
                price="Request-based"
                description="Limited evaluation access, proof artifact review, technical updates, and early demo availability for qualified evaluators."
              />
              <TierCard
                name="Design Partner / Paid Technical Evaluation"
                price="Scoped"
                description="Technical discovery, workload mapping, success criteria, prototype mapping where appropriate, and evaluation deliverables."
              />
              <TierCard
                name="Commercial Licensing"
                price="Agreement-based"
                description="Commercial use, support, hardware/IP, and enterprise licensing terms are handled through written agreements and counsel-reviewed documents."
              />
            </div>
          </Section>

          <Section title="3. Billing and Written Agreements">
            <ul className="list-disc list-inside space-y-2">
              <li>
                Paid technical evaluations and commercial engagements are scoped in writing.
              </li>
              <li>
                Payment timing, deliverables, and renewal terms are defined in the applicable agreement.
              </li>
              <li>
                Self-serve subscriptions and public free trials are not currently presented as the public evaluation model.
              </li>
              <li>
                Pricing, support, and license language should be reviewed in the relevant agreement before use.
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
                Open-source components in the public repository are governed by their
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
                Use restricted evaluation, SDK, IP, or licensing features without written authorization.
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
                Upon termination, your license to use restricted evaluation, SDK, IP, or licensing
                features ends immediately. You retain rights granted by applicable open-source
                licenses for public repository components.
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
