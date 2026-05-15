import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Get Started with ATOMiK", subtitle: "Request evaluation access, review proof artifacts, or map a workload.", accent: "#22c55e", badge: "Evaluation" });
}
