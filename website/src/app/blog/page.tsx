import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "Blog — ATOMiK",
  description: "Technical articles, release notes, and engineering insights from the ATOMiK team.",
};

const posts = [
  {
    slug: "atomik-vs-event-sourcing",
    title: "ATOMiK vs Event Sourcing: When XOR Beats Append-Only Logs",
    date: "March 16, 2026",
    excerpt:
      "A quantitative comparison of delta-state algebra vs event sourcing. O(1) reconstruction vs O(n) replay, 8 bytes vs unbounded logs, and when each approach wins.",
    tags: ["architecture", "comparison", "event-sourcing"],
  },
  {
    slug: "fpga-journey-13-dollar-chip",
    title: "From Math to Silicon: 69.7 Gops/s on a $13.50 Chip",
    date: "March 15, 2026",
    excerpt:
      "How we went from 92 Lean4 theorems to a custom RISC-V CPU with native delta-state instructions, HD HDMI output, and 69.7 billion operations per second on commodity FPGAs.",
    tags: ["hardware", "fpga", "risc-v", "engineering"],
  },
  {
    slug: "announcing-atomik-kernel-module",
    title: "Announcing ATOMiK v0.4.0: Per-Container Waste Tracking for Kubernetes",
    date: "March 15, 2026",
    excerpt:
      "The ATOMiK kernel module now detects redundant COW copies, deduplicates network sends, and attributes waste per container — all at kernel speed.",
    tags: ["release", "kernel", "kubernetes"],
  },
];

export default function BlogPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Blog" />

      <section className="max-w-3xl mx-auto px-6 pt-20 pb-8">
        <h1 className="text-4xl font-bold tracking-tight mb-2">Blog</h1>
        <p className="text-lg" style={{ color: "#8888a0" }}>
          Engineering insights, release notes, and the road to delta-state computing everywhere.
        </p>
      </section>

      <section className="max-w-3xl mx-auto px-6 pb-24">
        <div className="space-y-6">
          {posts.map((post) => (
            <Link
              key={post.slug}
              href={`/blog/${post.slug}`}
              className="block rounded-xl border p-6 transition-colors hover:border-[#4f8fff44]"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-center gap-3 mb-3">
                <time className="text-xs font-mono" style={{ color: "#8888a0" }}>
                  {post.date}
                </time>
                <div className="flex gap-2">
                  {post.tags.map((tag) => (
                    <span
                      key={tag}
                      className="text-xs px-2 py-0.5 rounded-full"
                      style={{
                        background: "rgba(79, 143, 255, 0.1)",
                        color: "#4f8fff",
                        border: "1px solid rgba(79, 143, 255, 0.2)",
                      }}
                    >
                      {tag}
                    </span>
                  ))}
                </div>
              </div>
              <h2 className="text-xl font-semibold mb-2 group-hover:text-[#4f8fff]">
                {post.title}
              </h2>
              <p className="text-sm leading-relaxed" style={{ color: "#b0b0c0" }}>
                {post.excerpt}
              </p>
            </Link>
          ))}
        </div>

        <div className="mt-12">
          <EmailCapture />
        </div>
      </section>
    </div>
  );
}
