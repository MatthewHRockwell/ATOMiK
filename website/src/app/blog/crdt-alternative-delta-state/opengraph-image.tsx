import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "CRDT Alternative: Delta-State Algebra", subtitle: "Same convergence, simpler implementation, 92 formal proofs.", accent: "#22c55e", badge: "Comparison" });
}
