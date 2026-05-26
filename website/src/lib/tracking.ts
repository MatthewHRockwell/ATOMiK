export type ContactIntent = "evaluation" | "demo" | "design-partner" | "investor" | "licensing" | "question" | "proof";

export function contactHref(intent: ContactIntent, source: string, cta: string) {
  const params = new URLSearchParams({ intent, source, cta });
  return `/contact?${params.toString()}`;
}
