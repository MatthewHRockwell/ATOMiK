"use client";

import { useEffect, useState, useRef } from "react";

/**
 * Animated counter that counts up when scrolled into view.
 * Used for hero stats, proof points, and metric strips.
 */
export function AnimatedCounter({
  value,
  suffix = "",
  prefix = "",
  duration = 1800,
}: {
  value: string;
  suffix?: string;
  prefix?: string;
  duration?: number;
}) {
  const ref = useRef<HTMLSpanElement>(null);
  const [visible, setVisible] = useState(false);
  const [display, setDisplay] = useState(prefix + "0" + suffix);
  const numericStr = value.replace(/[^0-9.]/g, "");
  const target = parseFloat(numericStr);
  const isNumeric = !isNaN(target);

  // Detect scroll into view
  useEffect(() => {
    const el = ref.current;
    if (!el) return;
    const observer = new IntersectionObserver(
      ([entry]) => {
        if (entry.isIntersecting) {
          setVisible(true);
          observer.disconnect();
        }
      },
      { threshold: 0.3 }
    );
    observer.observe(el);
    return () => observer.disconnect();
  }, []);

  // Animate once visible
  useEffect(() => {
    if (!visible) return;
    if (!isNumeric) return;

    const isDecimal = numericStr.includes(".");
    const trailingText = value.replace(numericStr, ""); // e.g., "%" or another unit suffix

    let start: number | null = null;
    let raf: number;

    function step(ts: number) {
      if (!start) start = ts;
      const progress = Math.min((ts - start) / duration, 1);
      const eased = 1 - Math.pow(1 - progress, 3);
      const current = eased * target;

      let formatted: string;
      if (isDecimal) {
        formatted = current.toFixed(1);
      } else if (target >= 1000) {
        formatted = Math.round(current).toLocaleString("en-US");
      } else {
        formatted = Math.round(current).toString();
      }

      setDisplay(prefix + formatted + trailingText + suffix);

      if (progress < 1) {
        raf = requestAnimationFrame(step);
      }
    }

    raf = requestAnimationFrame(step);
    return () => cancelAnimationFrame(raf);
  }, [visible, value, prefix, suffix, duration, isNumeric, numericStr, target]);

  return <span ref={ref}>{isNumeric ? display : prefix + value + suffix}</span>;
}
