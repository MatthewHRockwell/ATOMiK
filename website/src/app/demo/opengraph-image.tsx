import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Interactive Delta-State Demo", subtitle: "LOAD, ACCUM, READ, SWAP — try the algebra in your browser.", accent: "#22d3ee", badge: "Live Demo" });
}
