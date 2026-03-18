import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Changelog — ATOMiK Delta-State Computing",
  description:
    "Every ATOMiK release, from Python SDK to FPGA silicon. Detailed changelog with version history, features, and improvements.",
};

export default function ChangelogLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return <>{children}</>;
}
