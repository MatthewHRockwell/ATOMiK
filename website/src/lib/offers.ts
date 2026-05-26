export type PaidOffer = {
  id: string;
  name: string;
  shortName: string;
  priceCents: number;
  priceLabel: string;
  for: string;
  body: string;
  checkoutDescription: string;
  includes: string[];
  cta: string;
  contactHref: string;
};

export const paidOffers: PaidOffer[] = [
  {
    id: "proof-review-deposit",
    name: "Proof Review Reservation",
    shortName: "Proof Review",
    priceCents: 75000,
    priceLabel: "$750",
    for: "Teams, investors, and technical evaluators who need a proof-bound review before a deeper evaluation.",
    body: "Reserve focused review time around one workload, one constraint, and the evidence needed to decide whether ATOMiK is worth deeper evaluation.",
    checkoutDescription:
      "Proof-review reservation for an ATOMiK workload and evidence review. Applied toward a scoped evaluation if both sides continue.",
    includes: [
      "Workload and constraint intake",
      "Public proof packet and current validation boundary",
      "Claim-safe evidence map",
      "Fit/no-fit recommendation for the next evaluation step",
      "Reservation credited toward a scoped evaluation if both sides continue",
    ],
    cta: "Reserve Proof Review",
    contactHref: "/contact?intent=proof",
  },
  {
    id: "technical-evaluation-deposit",
    name: "Technical Evaluation Reservation",
    shortName: "Evaluation",
    priceCents: 250000,
    priceLabel: "$2,500",
    for: "Teams with a real state-heavy workload, current baseline, and budget owner for heat, power, bandwidth, latency, footprint, reliability, or compute-density pressure.",
    body: "Start a focused evaluation track with workload mapping, success criteria, and an evidence plan before any larger design-partner commitment.",
    checkoutDescription:
      "Reservation for ATOMiK technical evaluation scoping. Final scope, deliverables, and commercial terms are confirmed in writing.",
    includes: [
      "Technical discovery and workload map",
      "Baseline comparison and success criteria",
      "Metrics and evidence-tier plan",
      "Fit/no-fit readout and next-step recommendation",
      "Reservation credited toward the scoped evaluation agreement",
    ],
    cta: "Start Evaluation",
    contactHref: "/contact?intent=evaluation",
  },
];

export function getPaidOffer(id: unknown) {
  if (typeof id !== "string") return undefined;
  return paidOffers.find((offer) => offer.id === id);
}
