import { ImageResponse } from "next/og";

export const runtime = "edge";
export const alt = "ATOMiK — Delta-State Computing";
export const size = { width: 1200, height: 630 };
export const contentType = "image/png";

export default function OGImage() {
  return new ImageResponse(
    (
      <div
        style={{
          background: "linear-gradient(135deg, #0a0a0f 0%, #12121a 50%, #0a0a0f 100%)",
          width: "100%",
          height: "100%",
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          justifyContent: "center",
          fontFamily: "system-ui, sans-serif",
          position: "relative",
        }}
      >
        {/* Gradient accent line */}
        <div
          style={{
            position: "absolute",
            top: 0,
            left: 0,
            right: 0,
            height: 4,
            background: "linear-gradient(90deg, #8b5cf6, #4f8fff, #22d3ee)",
          }}
        />

        {/* Logo */}
        <div
          style={{
            display: "flex",
            fontSize: 72,
            fontWeight: 800,
            letterSpacing: -2,
            marginBottom: 20,
          }}
        >
          <span style={{ color: "#8b5cf6" }}>ATOM</span>
          <span style={{ color: "#4f8fff" }}>i</span>
          <span style={{ color: "#8b5cf6" }}>K</span>
        </div>

        {/* Tagline */}
        <div
          style={{
            fontSize: 32,
            color: "#e0e0e8",
            fontWeight: 600,
            marginBottom: 12,
          }}
        >
          Delta-State Computing
        </div>

        {/* Subtitle */}
        <div
          style={{
            fontSize: 20,
            color: "#8888a0",
            maxWidth: 700,
            textAlign: "center",
          }}
        >
          O(1) state reconstruction. 99% less bandwidth. 108 formal proofs.
        </div>

        {/* Stats bar */}
        <div
          style={{
            display: "flex",
            gap: 40,
            marginTop: 40,
          }}
        >
          {[
            { value: "69.7G", label: "ops/sec" },
            { value: "108", label: "proofs" },
            { value: "218+", label: "tests" },
          ].map((stat) => (
            <div
              key={stat.label}
              style={{
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
              }}
            >
              <span style={{ color: "#22d3ee", fontSize: 28, fontWeight: 700 }}>
                {stat.value}
              </span>
              <span style={{ color: "#666", fontSize: 14 }}>{stat.label}</span>
            </div>
          ))}
        </div>

        {/* URL */}
        <div
          style={{
            position: "absolute",
            bottom: 24,
            fontSize: 16,
            color: "#555",
          }}
        >
          atomik.tech
        </div>
      </div>
    ),
    { ...size }
  );
}
