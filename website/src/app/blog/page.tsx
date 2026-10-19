import type { Metadata } from "next";
import { redirect } from "next/navigation";

export const metadata: Metadata = {
  title: "Archived Route - ATOMiK",
  description: "This archived ATOMiK route now redirects to the current technical proof brief.",
  robots: { index: false, follow: false },
};

export default function ArchivedRouteRedirect() {
  redirect("/whitepaper");
}
