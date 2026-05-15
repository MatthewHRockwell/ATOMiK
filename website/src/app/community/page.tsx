"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

const communityChannels = [
  {
    title: "GitHub Discussions",
    description:
      "Ask questions, share patterns, get help from the community and maintainers.",
    color: "#4f8fff",
    icon: "GH",
    href: "https://github.com/MatthewHRockwell/ATOMiK/discussions",
    linkText: "Join the Discussion",
  },
  {
    title: "GitHub Issues",
    description:
      "Report bugs, request features, and track development progress.",
    color: "#22c55e",
    icon: "IS",
    href: "https://github.com/MatthewHRockwell/ATOMiK/issues",
    linkText: "File an Issue",
  },
  {
    title: "Stack Overflow",
    description:
      "Search for answers or ask new questions. Tag your posts with [atomik-sdk].",
    color: "#d4a843",
    icon: "SO",
    href: "https://stackoverflow.com/questions/tagged/atomik-sdk",
    linkText: "Browse Questions",
  },
];

const stats = [
  {
    value: "Open",
    label: "GitHub Discussions",
    sublabel: "Questions and patterns",
    gradient: "from-blue-400 to-blue-600",
    gradientColors: "#4f8fff, #3b82f6",
  },
  {
    value: "Proof-led",
    label: "Evidence Labels",
    sublabel: "Formal, software, hardware",
    gradientColors: "#a855f7, #7c3aed",
  },
  {
    value: "Artifact-led",
    label: "Validation",
    sublabel: "Builds, tests, logs",
    gradientColors: "#22c55e, #16a34a",
  },
  {
    value: "Request-based",
    label: "Evaluation Access",
    sublabel: "Qualified teams",
    gradientColors: "#22d3ee, #06b6d4",
  },
];

const contributionSteps = [
  {
    step: "01",
    title: "Fork & Clone",
    description:
      "Fork the ATOMiK repository on GitHub and clone it locally. The README has setup instructions for all supported platforms.",
    color: "#4f8fff",
  },
  {
    step: "02",
    title: "Pick an Issue",
    description:
      "Look for issues tagged 'good first issue' or 'help wanted'. These are specifically scoped for new contributors.",
    color: "#8b5cf6",
  },
  {
    step: "03",
    title: "Submit a PR",
    description:
      "Open a pull request with your changes. All PRs go through CI testing and code review before merge.",
    color: "#22c55e",
  },
];

export default function CommunityPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero Section */}
      <section className="relative overflow-hidden">
        <div
          className="absolute inset-0 opacity-20"
          style={{
            background:
              "radial-gradient(ellipse 60% 40% at 50% 0%, #4f8fff40, transparent), radial-gradient(ellipse 40% 30% at 20% 20%, #8b5cf630, transparent)",
          }}
        />
        <div className="max-w-5xl mx-auto px-6 pt-24 pb-16 relative">
          <p
            className="text-sm font-mono tracking-widest uppercase mb-4"
            style={{ color: "#4f8fff" }}
          >
            Community
          </p>
          <h1 className="text-5xl sm:text-6xl font-bold tracking-tight mb-6">
            Build with{" "}
            <span
              className="bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #4f8fff, #8b5cf6, #22c55e)",
              }}
            >
              ATOMiK
            </span>
          </h1>
          <p className="text-xl leading-relaxed max-w-3xl" style={{ color: "#8888a0" }}>
            Connect with developers exploring delta-state algebra. Ask questions, contribute
            code, and help shape the future of computing.
          </p>
        </div>
      </section>

      {/* Community Stats */}
      <section className="max-w-5xl mx-auto px-6 pb-16">
        <div className="grid grid-cols-2 lg:grid-cols-4 gap-6">
          {stats.map((stat) => (
            <div
              key={stat.label}
              className="rounded-xl p-6 border text-center"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="text-3xl sm:text-4xl font-bold mb-2 bg-clip-text text-transparent"
                style={{
                  backgroundImage: `linear-gradient(135deg, ${stat.gradientColors})`,
                }}
              >
                {stat.value}
              </div>
              <div className="font-semibold text-white text-sm mb-1">{stat.label}</div>
              <div className="text-xs" style={{ color: "#8888a0" }}>
                {stat.sublabel}
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Community Channels */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Join the Conversation</h2>
        <p className="mb-10 text-lg" style={{ color: "#8888a0" }}>
          Find help, share ideas, and connect with other developers.
        </p>

        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {communityChannels.map((channel) => (
            <a
              key={channel.title}
              href={channel.href}
              target="_blank"
              rel="noopener noreferrer"
              className="group rounded-xl p-8 border transition-all duration-300 hover:-translate-y-1 block"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="w-14 h-14 rounded-xl flex items-center justify-center text-lg font-bold font-mono mb-5"
                style={{ background: "#1e1e2e", color: channel.color }}
              >
                {channel.icon}
              </div>
              <h3 className="text-xl font-bold mb-3" style={{ color: channel.color }}>
                {channel.title}
              </h3>
              <p className="text-sm leading-relaxed mb-5" style={{ color: "#8888a0" }}>
                {channel.description}
              </p>
              <span
                className="inline-flex items-center gap-2 text-sm font-medium group-hover:gap-3 transition-all"
                style={{ color: channel.color }}
              >
                {channel.linkText}
                <span>&rarr;</span>
              </span>
            </a>
          ))}
        </div>
      </section>

      {/* Newsletter */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div
          className="rounded-2xl p-8 sm:p-12 border"
          style={{
            background: "linear-gradient(135deg, #12121a, #181824)",
            borderColor: "#1e1e2e",
          }}
        >
          <div className="flex flex-col lg:flex-row gap-8 items-start">
            <div className="flex-1">
              <h2 className="text-2xl font-bold mb-3">Newsletter</h2>
              <p className="leading-relaxed mb-6" style={{ color: "#b0b0c0" }}>
                Monthly updates on releases, benchmarks, and research. Stay informed about
                new SDK features, hardware results, and community highlights. No spam — ever.
              </p>
              <EmailCapture />
            </div>
            <div className="lg:w-72 shrink-0 w-full">
              <div
                className="rounded-xl p-6 border"
                style={{ background: "#0a0a0f", borderColor: "#1e1e2e" }}
              >
                <h4 className="font-semibold text-white text-sm mb-3">What you get</h4>
                <ul className="space-y-2">
                  {[
                    "Release announcements",
                    "Benchmark deep dives",
                    "Hardware build logs",
                    "Research highlights",
                  ].map((item) => (
                    <li
                      key={item}
                      className="flex items-center gap-2 text-sm"
                      style={{ color: "#8888a0" }}
                    >
                      <span style={{ color: "#22c55e" }}>+</span>
                      {item}
                    </li>
                  ))}
                </ul>
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* Contributing */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Contributing</h2>
        <p className="mb-10 text-lg" style={{ color: "#8888a0" }}>
          ATOMiK is open source. We welcome pull requests from the community.
        </p>

        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-8">
          {contributionSteps.map((item) => (
            <div
              key={item.step}
              className="rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="text-4xl font-bold font-mono mb-3 opacity-30"
                style={{ color: item.color }}
              >
                {item.step}
              </div>
              <h3 className="text-lg font-bold mb-2" style={{ color: item.color }}>
                {item.title}
              </h3>
              <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
                {item.description}
              </p>
            </div>
          ))}
        </div>

        <div className="flex flex-wrap gap-4">
          <Link
            href="https://github.com/MatthewHRockwell/ATOMiK"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 px-5 py-3 rounded-lg font-semibold text-white transition-all duration-300 hover:scale-105"
            style={{
              background: "linear-gradient(135deg, #4f8fff, #8b5cf6)",
            }}
          >
            View on GitHub &rarr;
          </Link>
          <Link
            href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/CONTRIBUTING.md"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 px-5 py-3 rounded-lg font-medium text-sm transition-colors hover:bg-white/5"
            style={{ border: "1px solid #1e1e2e", color: "#4f8fff" }}
          >
            Contributing Guidelines &rarr;
          </Link>
        </div>
      </section>

      {/* Code of Conduct */}
      <section className="max-w-5xl mx-auto px-6 pb-24">
        <div
          className="rounded-xl p-8 border flex flex-col sm:flex-row items-start sm:items-center gap-6"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <div
            className="w-14 h-14 rounded-full flex items-center justify-center text-xl font-bold shrink-0"
            style={{ background: "rgba(139, 92, 246, 0.15)", color: "#8b5cf6" }}
          >
            CC
          </div>
          <div className="flex-1">
            <h3 className="text-lg font-bold text-white mb-1">Code of Conduct</h3>
            <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
              ATOMiK follows the{" "}
              <a
                href="https://www.contributor-covenant.org/version/2/1/code_of_conduct/"
                target="_blank"
                rel="noopener noreferrer"
                className="hover:underline"
                style={{ color: "#8b5cf6" }}
              >
                Contributor Covenant v2.1
              </a>
              . We are committed to providing a welcoming and inclusive environment for everyone.
              Harassment and discrimination are not tolerated.
            </p>
          </div>
        </div>
      </section>
    </div>
  );
}
