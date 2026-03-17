import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK v0.4.0: Per-Container Waste Tracking", subtitle: "COW detection, network dedup, cgroup attribution — all at kernel speed.", accent: "#22c55e", badge: "Release" });
}
