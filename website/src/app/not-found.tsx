import Link from "next/link";
import Nav from "@/components/Nav";

export default function NotFound() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />
      <div className="max-w-2xl mx-auto px-6 pt-32 pb-24 text-center">
        <div className="text-6xl font-bold mb-4 font-mono" style={{ color: "#4f8fff" }}>
          404
        </div>
        <h1 className="text-2xl font-bold mb-4">Page not found</h1>
        <p className="mb-8" style={{ color: "#8888a0" }}>
          The state you&apos;re looking for doesn&apos;t exist at this address.
          Perhaps it needs a LOAD operation first.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/"
            className="px-6 py-2.5 rounded-lg text-sm font-semibold"
            style={{ background: "#4f8fff", color: "#fff" }}
          >
            Go Home
          </Link>
          <Link
            href="/docs"
            className="px-6 py-2.5 rounded-lg text-sm font-semibold"
            style={{ background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }}
          >
            Read the Docs
          </Link>
          <Link
            href="/investor-brief"
            className="px-6 py-2.5 rounded-lg text-sm font-semibold"
            style={{ background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }}
          >
            Investor Brief
          </Link>
        </div>
      </div>
    </div>
  );
}
