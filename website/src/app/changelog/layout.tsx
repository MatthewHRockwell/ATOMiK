import { pageMetadata } from "@/lib/siteMetadata";

export const metadata = pageMetadata({
  title: 'Changelog — ATOMiK Evidence-Labeled Updates',
  description: 'Evidence-labeled ATOMiK updates from software proof to FPGA hardware artifacts, with current status separated from historical notes.',
  path: '/changelog',
  openGraphTitle: 'ATOMiK Evidence-Labeled Changelog',
});

export default function ChangelogLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return <>{children}</>;
}
