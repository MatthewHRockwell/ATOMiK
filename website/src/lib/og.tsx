import { ImageResponse } from "next/og";

export const ogSize = { width: 1200, height: 630 };

/**
 * Generate an ATOMiK-branded OG image with custom title and subtitle.
 * Used by opengraph-image.tsx files in each route.
 */
export function generateOGImage({
  title,
  subtitle,
  accent = "#4f8fff",
  badge,
}: {
  title: string;
  subtitle?: string;
  accent?: string;
  badge?: string;
}) {
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
          padding: 60,
        }}
      >
        {/* Top accent */}
        <div
          style={{
            position: "absolute",
            top: 0,
            left: 0,
            right: 0,
            height: 4,
            background: `linear-gradient(90deg, #8b5cf6, ${accent}, #22d3ee)`,
          }}
        />

        {/* Logo */}
        <div
          style={{
            display: "flex",
            fontSize: 36,
            fontWeight: 800,
            letterSpacing: -1,
            marginBottom: 32,
          }}
        >
          <span style={{ color: "#8b5cf6" }}>ATOM</span>
          <span style={{ color: "#4f8fff" }}>i</span>
          <span style={{ color: "#8b5cf6" }}>K</span>
        </div>

        {/* Badge */}
        {badge && (
          <div
            style={{
              fontSize: 14,
              color: accent,
              textTransform: "uppercase",
              letterSpacing: 2,
              fontWeight: 700,
              marginBottom: 16,
              padding: "6px 16px",
              border: `1px solid ${accent}44`,
              borderRadius: 999,
              background: `${accent}11`,
            }}
          >
            {badge}
          </div>
        )}

        {/* Title */}
        <div
          style={{
            fontSize: 48,
            fontWeight: 700,
            color: "#e0e0e8",
            textAlign: "center",
            lineHeight: 1.2,
            maxWidth: 900,
            marginBottom: subtitle ? 16 : 0,
          }}
        >
          {title}
        </div>

        {/* Subtitle */}
        {subtitle && (
          <div
            style={{
              fontSize: 22,
              color: "#8888a0",
              textAlign: "center",
              maxWidth: 750,
            }}
          >
            {subtitle}
          </div>
        )}

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
    { ...ogSize }
  );
}
