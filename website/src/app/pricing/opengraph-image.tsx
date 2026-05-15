import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Evaluation", subtitle: "Request evaluation access or discuss a scoped design-partner engagement.", accent: "#4f8fff", badge: "Evaluation" });
}
