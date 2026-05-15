import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK v0.4.0: Per-Container Waste Tracking", subtitle: "Archived kernel-module prototype note with evidence-scoped claims.", accent: "#22c55e", badge: "Release" });
}
