"use client";

import { useState } from "react";

type CheckoutButtonProps = {
  offerId: string;
  children: React.ReactNode;
  className?: string;
  wrapperClassName?: string;
  style?: React.CSSProperties;
  source?: string;
  cta?: string;
};

export default function CheckoutButton({
  offerId,
  children,
  className,
  wrapperClassName,
  style,
  source = "pricing",
  cta,
}: CheckoutButtonProps) {
  const [status, setStatus] = useState<"idle" | "loading" | "error">("idle");

  async function startCheckout() {
    setStatus("loading");

    try {
      const res = await fetch("/api/checkout", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          offerId,
          source,
          cta: cta || (typeof children === "string" ? children : offerId),
          landing_path: window.location.pathname + window.location.search,
          referrer: document.referrer,
          utm_source: new URLSearchParams(window.location.search).get("utm_source") || "",
          utm_medium: new URLSearchParams(window.location.search).get("utm_medium") || "",
          utm_campaign: new URLSearchParams(window.location.search).get("utm_campaign") || "",
          utm_content: new URLSearchParams(window.location.search).get("utm_content") || "",
          utm_term: new URLSearchParams(window.location.search).get("utm_term") || "",
        }),
      });
      const data = (await res.json()) as { url?: string; error?: string };

      if (!res.ok || !data.url) {
        setStatus("error");
        return;
      }

      window.location.assign(data.url);
    } catch {
      setStatus("error");
    }
  }

  return (
    <div className={wrapperClassName}>
      <button
        type="button"
        disabled={status === "loading"}
        onClick={startCheckout}
        className={className}
        style={style}
      >
        {status === "loading" ? "Opening Stripe..." : children}
      </button>
      {status === "error" && (
        <p className="mt-2 text-xs leading-5" style={{ color: "#fca5a5" }}>
          Checkout did not open. Use the contact path and we will follow up directly.
        </p>
      )}
    </div>
  );
}
