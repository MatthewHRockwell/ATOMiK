import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "5 Design Patterns for Delta-State Algebra", subtitle: "Fan-in, checkpointing, fingerprint gates, rollback chains, multi-stream.", accent: "#8b5cf6", badge: "Patterns" });
}
