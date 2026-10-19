import type { Metadata } from "next";
import { redirect } from "next/navigation";

export const metadata: Metadata = {
  title: "Archived Demo Route - ATOMiK",
  description: "This archived ATOMiK demo route now redirects to the current hardware proof map.",
  robots: { index: false, follow: false },
};

export default function ArchivedDemoRedirect() {
  redirect("/docs/hardware");
}
