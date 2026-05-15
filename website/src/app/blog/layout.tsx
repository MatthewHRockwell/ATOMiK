import EvidenceBanner from "@/components/EvidenceBanner";

export default function BlogLayout({ children }: { children: React.ReactNode }) {
  return (
    <>
      <div style={{ background: "#0a0a0f", paddingTop: "0.75rem" }}>
        <EvidenceBanner surface="blog" />
      </div>
      {children}
    </>
  );
}
