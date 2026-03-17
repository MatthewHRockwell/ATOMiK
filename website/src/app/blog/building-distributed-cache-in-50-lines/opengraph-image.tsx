import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Build a Distributed Cache in 50 Lines", subtitle: "3-node cache with automatic convergence. No consensus, no leader election.", accent: "#22d3ee", badge: "Tutorial" });
}
