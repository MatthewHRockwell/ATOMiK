import type { Metadata } from "next";
import {
  codeBlockStyle,
  kwColor,
  fnColor,
  strColor,
  numColor,
  cmtColor,
  typeColor,
  varColor,
  DocsNav,
} from "../shared";

export const metadata: Metadata = {
  title: "Kernel Module — ATOMiK Docs",
  description:
    "Install and use the ATOMiK Linux kernel module. DKMS-managed, supports kernels 5.15+. ioctl and sysfs interface.",
};

export default function KernelModulePage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#d4a843" }}
      >
        System Integration
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">Kernel Module</h1>
      <p className="text-lg mb-10" style={{ color: "#8888a0" }}>
        The ATOMiK Linux kernel module exposes delta-state operations via{" "}
        <code
          className="text-sm font-mono px-2 py-0.5 rounded"
          style={{ background: "#1e1e2e", color: "#d4a843" }}
        >
          /dev/atomik
        </code>{" "}
        and sysfs. DKMS-managed, supports kernels 5.15+.
      </p>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
        {/* Install + Load */}
        <div>
          <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
            Install &amp; load
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: cmtColor }}># Clone and install via DKMS</span>
                {"\n"}
                <span style={{ color: cmtColor }}>$</span>{" "}
                <span style={{ color: varColor }}>
                  git clone https://github.com/MatthewHRockwell/ATOMiK
                </span>
                {"\n"}
                <span style={{ color: cmtColor }}>$</span>{" "}
                <span style={{ color: kwColor }}>cd</span>{" "}
                <span style={{ color: varColor }}>ATOMiK/software/atomik_kmod</span>
                {"\n"}
                <span style={{ color: cmtColor }}>$</span>{" "}
                <span style={{ color: kwColor }}>sudo</span>{" "}
                <span style={{ color: varColor }}>./install.sh</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Verify it loaded</span>
                {"\n"}
                <span style={{ color: cmtColor }}>$</span>{" "}
                <span style={{ color: varColor }}>
                  cat /sys/class/atomik/atomik0/version
                </span>
                {"\n"}
                <span style={{ color: strColor }}>0.4.0</span>
              </code>
            </pre>
          </div>
        </div>

        {/* Sysfs interface */}
        <div>
          <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
            sysfs interface
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: cmtColor }}># Runtime status</span>
                {"\n"}
                <span style={{ color: cmtColor }}>$</span>{" "}
                <span style={{ color: varColor }}>
                  cat /sys/class/atomik/atomik0/backend
                </span>
                {"\n"}
                <span style={{ color: strColor }}>software</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Operation counters</span>
                {"\n"}
                <span style={{ color: cmtColor }}>$</span>{" "}
                <span style={{ color: varColor }}>
                  cat /sys/class/atomik/atomik0/ops_total
                </span>
                {"\n"}
                <span style={{ color: numColor }}>0</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Per-operation: ops_load, ops_accum,</span>
                {"\n"}
                <span style={{ color: cmtColor }}># ops_read, ops_swap</span>
              </code>
            </pre>
          </div>
        </div>
      </div>

      {/* ioctl example */}
      <div className="mb-6">
        <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
          ioctl usage (C)
        </p>
        <div style={codeBlockStyle}>
          <pre style={{ margin: 0 }}>
            <code>
              <span style={{ color: kwColor }}>#include</span>{" "}
              <span style={{ color: strColor }}>&lt;uapi/atomik.h&gt;</span>
              {"\n\n"}
              <span style={{ color: typeColor }}>int</span>{" "}
              <span style={{ color: varColor }}>fd</span>{" "}
              <span style={{ color: kwColor }}>=</span>{" "}
              <span style={{ color: fnColor }}>open</span>
              <span style={{ color: varColor }}>(</span>
              <span style={{ color: strColor }}>"/dev/atomik"</span>
              <span style={{ color: varColor }}>, O_RDWR);</span>
              {"\n\n"}
              <span style={{ color: cmtColor }}>// Create a table with 256 contexts</span>
              {"\n"}
              <span style={{ color: kwColor }}>struct</span>{" "}
              <span style={{ color: typeColor }}>atomik_create_table_args</span>{" "}
              <span style={{ color: varColor }}>ct</span>{" "}
              <span style={{ color: kwColor }}>=</span>{" "}
              <span style={{ color: varColor }}>{"{"}</span>{" "}
              <span style={{ color: varColor }}>.num_contexts</span>{" "}
              <span style={{ color: kwColor }}>=</span>{" "}
              <span style={{ color: numColor }}>256</span>{" "}
              <span style={{ color: varColor }}>{"}"}</span>
              <span style={{ color: varColor }}>;</span>
              {"\n"}
              <span style={{ color: fnColor }}>ioctl</span>
              <span style={{ color: varColor }}>(fd, ATOMIK_IOC_CREATE_TABLE, &amp;ct);</span>
              {"\n\n"}
              <span style={{ color: cmtColor }}>// Core operations via ioctl</span>
              {"\n"}
              <span style={{ color: kwColor }}>struct</span>{" "}
              <span style={{ color: typeColor }}>atomik_load_args</span>{" "}
              <span style={{ color: varColor }}>la</span>{" "}
              <span style={{ color: kwColor }}>=</span>{" "}
              <span style={{ color: varColor }}>
                {"{"} .table_id = ct.table_id,
              </span>
              {"\n"}
              <span style={{ color: varColor }}>
                {"    "}.addr = <span style={{ color: numColor }}>0</span>, .initial_state ={" "}
                <span style={{ color: numColor }}>0xDEADBEEF</span> {"}"};
              </span>
              {"\n"}
              <span style={{ color: fnColor }}>ioctl</span>
              <span style={{ color: varColor }}>(fd, ATOMIK_IOC_LOAD, &amp;la);</span>
            </code>
          </pre>
        </div>
      </div>

      <DocsNav
        prev={{ href: "/docs/quickstart", label: "Quick Start" }}
        next={{ href: "/docs/api", label: "API Reference" }}
      />
    </div>
  );
}
