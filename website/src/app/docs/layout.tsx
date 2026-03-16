import Nav from "@/components/Nav";
import DocsSidebar from "@/components/DocsSidebar";

export default function DocsLayout({ children }: { children: React.ReactNode }) {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Docs" />
      <DocsSidebar />

      {/* Main content area — pushed right on desktop to clear the sidebar */}
      <main className="lg:ml-[240px]">{children}</main>
    </div>
  );
}
