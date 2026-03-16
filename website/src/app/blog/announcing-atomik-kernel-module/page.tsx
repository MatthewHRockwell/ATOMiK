import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Announcing ATOMiK v0.4.0: Per-Container Waste Tracking — ATOMiK Blog",
  description:
    "The ATOMiK kernel module now detects redundant COW copies, deduplicates network sends, and attributes waste per Kubernetes container.",
};

function Code({ children }: { children: string }) {
  return (
    <pre
      className="rounded-lg p-4 text-sm overflow-x-auto my-4"
      style={{
        background: "#0a0a0f",
        border: "1px solid #1e1e2e",
        color: "#22d3ee",
        fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
        lineHeight: 1.6,
      }}
    >
      <code>{children}</code>
    </pre>
  );
}

function InlineCode({ children }: { children: string }) {
  return (
    <code
      className="px-1.5 py-0.5 rounded text-sm"
      style={{
        background: "rgba(79, 143, 255, 0.1)",
        color: "#22d3ee",
        fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
      }}
    >
      {children}
    </code>
  );
}

export default function BlogPost() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Blog" />

      <article className="max-w-3xl mx-auto px-6 pt-16 pb-24">
        {/* Header */}
        <div className="mb-12">
          <Link
            href="/blog"
            className="text-sm mb-6 inline-block hover:underline"
            style={{ color: "#4f8fff" }}
          >
            &larr; Back to blog
          </Link>
          <time className="block text-sm font-mono mb-3" style={{ color: "#8888a0" }}>
            March 15, 2026
          </time>
          <h1 className="text-3xl font-bold tracking-tight mb-4 leading-tight">
            Announcing ATOMiK v0.4.0: Per-Container Waste Tracking for Kubernetes
          </h1>
          <div className="flex gap-2">
            {["release", "kernel", "kubernetes"].map((tag) => (
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

        {/* Content */}
        <div className="space-y-6 text-base leading-relaxed" style={{ color: "#c8c8d4" }}>
          <p>
            Every fork(), every COW fault, every TCP retransmit — your kernel is doing work
            that may be completely redundant. ATOMiK v0.4.0 makes that waste visible,
            measurable, and attributable to the exact container that caused it.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            What is ATOMiK?
          </h2>
          <p>
            ATOMiK is a delta-state algebra — four operations (<InlineCode>LOAD</InlineCode>,{" "}
            <InlineCode>ACCUM</InlineCode>, <InlineCode>READ</InlineCode>,{" "}
            <InlineCode>SWAP</InlineCode>) that replace snapshots, event replay, and
            full-state replication. Instead of storing state, you reconstruct it:
          </p>
          <Code>{`current_state = initial_state XOR accumulator`}</Code>
          <p>
            This is backed by 92 formally proven Lean4 theorems (commutativity, associativity,
            self-inverse, identity) and runs on everything from a Python pip install to custom
            FPGA silicon hitting 69.7 billion operations per second.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            What&apos;s new in v0.4.0
          </h2>

          <h3 className="text-xl font-semibold pt-2" style={{ color: "#e0e0e8" }}>
            1. COW Redundancy Detection
          </h3>
          <p>
            The kernel module installs a <InlineCode>kretprobe</InlineCode> on{" "}
            <InlineCode>wp_page_copy()</InlineCode> to fingerprint every copy-on-write page
            before and after the copy. If the content is identical — the copy was wasted work.
          </p>
          <Code>{`$ cat /sys/class/atomik/atomik0/cow_redundancy_rate
23.4%`}</Code>
          <p>
            That 23.4% means nearly a quarter of all COW faults in your system are copying
            pages that haven&apos;t actually changed. On a busy Kubernetes node with hundreds of
            pods forking workers, this adds up fast.
          </p>

          <h3 className="text-xl font-semibold pt-2" style={{ color: "#e0e0e8" }}>
            2. Network Send Deduplication
          </h3>
          <p>
            A kprobe on <InlineCode>tcp_sendmsg()</InlineCode> fingerprints every outgoing TCP
            buffer using hardware-accelerated CRC32C (SSE4.2). Consecutive identical sends to
            the same socket are flagged as redundant.
          </p>
          <Code>{`$ cat /sys/class/atomik/atomik0/net_redundancy_rate
8.7%`}</Code>
          <p>
            Common culprits: health check responses, polling APIs returning unchanged JSON,
            status endpoints serving cached data.
          </p>

          <h3 className="text-xl font-semibold pt-2" style={{ color: "#e0e0e8" }}>
            3. Per-Container Waste Attribution
          </h3>
          <p>
            The headline feature. Every COW fault and network send is tagged with the calling
            task&apos;s cgroup via <InlineCode>task_dfl_cgroup()</InlineCode>. Waste is aggregated
            per container and exposed via sysfs:
          </p>
          <Code>{`$ cat /sys/class/atomik/atomik0/cgroup_top
Container               COW Faults    COW Waste    Net Sends   Net Waste
api-server-7f8d4        1,234         12.5 MB      5,678       2.3 MB
worker-pool-3a2c        891           8.9 MB       3,210       1.1 MB
redis-cache-9e1b        234           2.3 MB       12,456      890 KB
monitoring-agent        170           1.7 MB       8,901       456 KB`}</Code>
          <p>
            For Kubernetes: each pod maps to its own cgroup. For Docker: each container gets
            its own entry under <InlineCode>system.slice</InlineCode>. The insight is instant
            and zero-overhead — no eBPF programs, no userspace polling, no sampling.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            27 Real-Time Metrics
          </h2>
          <p>
            The full sysfs interface exposes 27 attributes under{" "}
            <InlineCode>/sys/class/atomik/atomik0/</InlineCode>:
          </p>
          <ul className="list-disc pl-6 space-y-1">
            <li>Core operations: ops_total, ops_load, ops_accum, ops_read, ops_swap</li>
            <li>Fingerprinting: fp_checks_total, fp_hit_rate, fp_bytes_saved</li>
            <li>COW: cow_faults_total, cow_redundancy_rate, cow_top (per-process)</li>
            <li>Network: net_sends_total, net_redundancy_rate</li>
            <li>Containers: cgroup_top (per-container breakdown)</li>
            <li>License: license_status (trial countdown or active)</li>
          </ul>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Install in 60 seconds
          </h2>
          <Code>{`# Download and install (free 90-day trial)
curl -sL https://atomik.tech/download/atomik-0.4.0.tar.gz | tar xz
cd atomik-0.4.0
sudo ./install.sh

# Check it's working
sudo atomik-status`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Or start with the Python SDK
          </h2>
          <Code>{`pip install atomik-core

# Run the benchmark on your machine
python -m atomik_core.benchmark`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            What&apos;s next
          </h2>
          <p>
            v0.4.0 detects waste. The next release will start <em>eliminating</em> it —
            COW copy suppression, network delta compression, and automated optimization
            recommendations. The kernel module already knows which copies are redundant;
            the next step is preventing them.
          </p>
          <p>
            We&apos;re also bringing up ATOMiK on Xilinx Zynq for hardware-accelerated
            delta processing at 69.7 Gops/s — but that&apos;s a story for another post.
          </p>

          {/* CTA */}
          <div
            className="rounded-xl border p-8 mt-8 text-center"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <h3 className="text-xl font-bold mb-3">Ready to see your waste?</h3>
            <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
              Free 90-day trial. No credit card required.
            </p>
            <div className="flex flex-wrap justify-center gap-4">
              <Link
                href="/get-started"
                className="px-6 py-2.5 rounded-lg text-sm font-semibold"
                style={{ background: "#4f8fff", color: "#fff" }}
              >
                Get Started
              </Link>
              <Link
                href="https://github.com/MatthewHRockwell/ATOMiK"
                className="px-6 py-2.5 rounded-lg text-sm font-semibold"
                style={{
                  background: "transparent",
                  color: "#e0e0e8",
                  border: "1px solid #1e1e2e",
                }}
              >
                View on GitHub
              </Link>
            </div>
          </div>
        </div>
      </article>
    </div>
  );
}
