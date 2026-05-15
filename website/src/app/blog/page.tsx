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
    slug: "what-is-delta-state-computing",
    title: "What is Delta-State Computing? The Definitive Guide",
    date: "March 14, 2026",
    excerpt:
      "An archived guide to delta-state architecture: mathematical foundations, comparisons with CRDTs, event sourcing, OT, and Raft, and where reconstruction may help.",
    tags: ["delta-state", "architecture", "pillar"],
  },
  {
    slug: "crdt-alternative-delta-state",
    title: "CRDT Alternative: Delta-State Algebra",
    date: "March 12, 2026",
    excerpt:
      "CRDTs are powerful but complex. Delta-state algebra offers a different modeled convergence path with one core operation and machine-checked algebra proofs.",
    tags: ["CRDTs", "comparison", "pillar"],
  },
  {
    slug: "5-patterns-delta-state",
    title: "5 Design Patterns for Delta-State Algebra",
    date: "March 10, 2026",
    excerpt:
      "Practical patterns: accumulator fan-in, epoch checkpointing, fingerprint gates, rollback chains, and multi-stream convergence.",
    tags: ["patterns", "architecture", "python"],
  },
  {
    slug: "building-distributed-cache-in-50-lines",
    title: "Build a Distributed Cache in 50 Lines of Python",
    date: "March 7, 2026",
    excerpt:
      "Step-by-step tutorial: a 3-node cache model using XOR deltas. Treat it as an educational pattern, not a production distributed-systems guarantee.",
    tags: ["tutorial", "python", "distributed-systems"],
  },
  {
    slug: "atomik-vs-event-sourcing",
    title: "ATOMiK vs Event Sourcing: When XOR Beats Append-Only Logs",
    date: "March 4, 2026",
    excerpt:
      "A conceptual comparison of delta-state algebra and event sourcing: where an accumulator model may help, and where append-only logs remain the right tool.",
    tags: ["architecture", "comparison", "event-sourcing"],
  },
  {
    slug: "fpga-journey-13-dollar-chip",
    title: "From Math to Silicon: FPGA Proof Notes",
    date: "February 28, 2026",
    excerpt:
      "An archived engineering note on moving from Lean4 proofs to custom RISC-V instructions, HDMI output, and synthesis-characterized FPGA scaling.",
    tags: ["hardware", "fpga", "risc-v", "engineering"],
  },
  {
    slug: "announcing-atomik-kernel-module",
    title: "Announcing ATOMiK v0.4.0: Per-Container Waste Tracking for Kubernetes",
    date: "February 21, 2026",
    excerpt:
      "Archived release note for a kernel-module prototype exploring redundant COW-copy detection, network-send analysis, and per-container attribution.",
    tags: ["release", "kernel", "kubernetes"],
  },
];

const blogJsonLd = {
  "@context": "https://schema.org",
  "@type": "ItemList",
  itemListElement: posts.map((post, i) => ({
    "@type": "ListItem",
    position: i + 1,
    item: {
      "@type": "Article",
      headline: post.title,
      datePublished: new Date(post.date).toISOString().split("T")[0],
      url: `https://atomik.tech/blog/${post.slug}`,
      description: post.excerpt,
      author: { "@type": "Person", name: "Matt Rockwell" },
    },
  })),
};

export default function BlogPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <script
        type="application/ld+json"
        dangerouslySetInnerHTML={{ __html: JSON.stringify(blogJsonLd) }}
      />
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
