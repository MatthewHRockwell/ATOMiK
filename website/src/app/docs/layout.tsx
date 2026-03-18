import Nav from "@/components/Nav";
import DocsSidebar from "@/components/DocsSidebar";
import { DocsBreadcrumb, DocsPrevNext } from "@/components/DocsNav";

const breadcrumbJsonLd = {
  "@context": "https://schema.org",
  "@type": "BreadcrumbList",
  itemListElement: [
    {
      "@type": "ListItem",
      position: 1,
      name: "Home",
      item: "https://atomik.tech",
    },
    {
      "@type": "ListItem",
      position: 2,
      name: "Docs",
      item: "https://atomik.tech/docs",
    },
  ],
};

export default function DocsLayout({ children }: { children: React.ReactNode }) {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <script
        type="application/ld+json"
        dangerouslySetInnerHTML={{ __html: JSON.stringify(breadcrumbJsonLd) }}
      />
      <Nav active="Docs" />
      <DocsSidebar />

      {/* Main content area -- pushed right on desktop to clear the sidebar */}
      <main className="lg:ml-[240px]">
        <div className="max-w-5xl mx-auto px-6 pt-8">
          <DocsBreadcrumb />
        </div>
        {children}
        <div className="max-w-5xl mx-auto px-6 pb-16">
          <DocsPrevNext />
        </div>
      </main>
    </div>
  );
}
