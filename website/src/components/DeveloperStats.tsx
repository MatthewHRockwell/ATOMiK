"use client";

import { useEffect, useState, useRef } from "react";

const stats = [
  {
    label: "Developers Registered",
    target: 247,
    suffix: "",
    color: "#4f8fff",
    bg: "rgba(79,143,255,0.10)",
    border: "rgba(79,143,255,0.25)",
  },
  {
    label: "PyPI Downloads",
    target: 1200,
    suffix: "+",
    color: "#22d3ee",
    bg: "rgba(34,211,238,0.10)",
    border: "rgba(34,211,238,0.25)",
  },
  {
    label: "SDK Tests Passing",
    target: 218,
    suffix: "",
    color: "#22c55e",
    bg: "rgba(34,197,94,0.10)",
    border: "rgba(34,197,94,0.25)",
  },
];

function formatNumber(n: number): string {
  return n.toLocaleString("en-US");
}

function useCountUp(target: number, duration: number, startCounting: boolean) {
  const [value, setValue] = useState(0);

  useEffect(() => {
    if (!startCounting) return;

    let start: number | null = null;
    let raf: number;

    function step(timestamp: number) {
      if (!start) start = timestamp;
      const elapsed = timestamp - start;
      const progress = Math.min(elapsed / duration, 1);
      // Ease-out cubic
      const eased = 1 - Math.pow(1 - progress, 3);
      setValue(Math.round(eased * target));

      if (progress < 1) {
        raf = requestAnimationFrame(step);
      }
    }

    raf = requestAnimationFrame(step);
    return () => cancelAnimationFrame(raf);
  }, [target, duration, startCounting]);

  return value;
}

function StatCard({
  stat,
  visible,
}: {
  stat: (typeof stats)[number];
  visible: boolean;
}) {
  const value = useCountUp(stat.target, 1600, visible);

  return (
    <div
      className="flex-1 min-w-[160px] rounded-xl px-6 py-5 text-center transition-all"
      style={{
        background: stat.bg,
        border: `1px solid ${stat.border}`,
      }}
    >
      <div
        className="text-3xl md:text-4xl font-extrabold tabular-nums"
        style={{ color: stat.color }}
      >
        {formatNumber(value)}
        {stat.suffix}
      </div>
      <div
        className="text-xs mt-1.5 font-medium tracking-wide uppercase"
        style={{ color: "#8888a0" }}
      >
        {stat.label}
      </div>
    </div>
  );
}

export default function DeveloperStats() {
  const ref = useRef<HTMLDivElement>(null);
  const [visible, setVisible] = useState(false);

  useEffect(() => {
    if (!ref.current) return;

    const observer = new IntersectionObserver(
      ([entry]) => {
        if (entry.isIntersecting) {
          setVisible(true);
          observer.disconnect();
        }
      },
      { threshold: 0.3 }
    );

    observer.observe(ref.current);
    return () => observer.disconnect();
  }, []);

  return (
    <div ref={ref} className="max-w-4xl mx-auto px-6">
      <div className="flex flex-col sm:flex-row gap-4 justify-center">
        {stats.map((stat) => (
          <StatCard key={stat.label} stat={stat} visible={visible} />
        ))}
      </div>
    </div>
  );
}
