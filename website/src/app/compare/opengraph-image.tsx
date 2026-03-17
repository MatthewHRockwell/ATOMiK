import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "How ATOMiK Compares", subtitle: "vs Redis, Kafka, CRDTs, Event Sourcing, Checksums, and GPUs.", accent: "#f59e0b", badge: "Comparison" });
}
