import Link from "next/link";

const tierColors: Record<string, string> = {
  pro: "#4f8fff",
  team: "#22d3ee",
  enterprise: "#8b5cf6",
};

const tierLabels: Record<string, string> = {
  pro: "Pro",
  team: "Team",
  enterprise: "Enterprise",
};

export default function UpgradeGate({
  tier,
  title,
  description,
  ctaText,
}: {
  tier: "pro" | "team" | "enterprise";
  title: string;
  description: string;
  ctaText: string;
}) {
  const color = tierColors[tier];
  const label = tierLabels[tier];

  return (
    <div
      className="rounded-xl overflow-hidden my-10"
      style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
    >
      {/* Gradient top bar */}
      <div
        className="h-1"
        style={{
          background: `linear-gradient(90deg, ${color}, ${color}88)`,
        }}
      />

      <div className="px-6 py-6">
        {/* Tier badge */}
        <span
          className="inline-block text-xs font-mono font-semibold tracking-wider uppercase px-2.5 py-1 rounded-md mb-4"
          style={{
            background: `${color}18`,
            color,
            border: `1px solid ${color}40`,
          }}
        >
          {label}
        </span>

        <h3 className="text-lg font-bold mb-2" style={{ color: "#e0e0e8" }}>
          {title}
        </h3>

        <p className="text-sm mb-5 leading-relaxed" style={{ color: "#8888a0" }}>
          {description}
        </p>

        <Link
          href={`/register?plan=${tier}`}
          className="inline-block text-sm font-semibold no-underline px-5 py-2.5 rounded-lg transition-opacity hover:opacity-90"
          style={{ background: color, color: "#fff" }}
        >
          {ctaText}
        </Link>
      </div>
    </div>
  );
}
