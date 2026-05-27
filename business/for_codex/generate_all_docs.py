#!/usr/bin/env python3
"""Generate all investor package documents in proper formats.
Run with: /home/mattrock/Projects/ATOMiK/.venv/bin/python generate_all_docs.py
"""

import shutil
from pathlib import Path
from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
import openpyxl
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

ROOT = Path(__file__).parent
REPO = ROOT.parent.parent

# ─── STYLE HELPERS ───────────────────────────────────────────────────────────

def styled_doc(title):
    doc = Document()
    # Margins
    for section in doc.sections:
        section.top_margin = Inches(1.0)
        section.bottom_margin = Inches(1.0)
        section.left_margin = Inches(1.2)
        section.right_margin = Inches(1.2)

    # Title
    h = doc.add_heading(title, 0)
    h.runs[0].font.color.rgb = RGBColor(0x07, 0x0B, 0x12)
    h.runs[0].font.size = Pt(24)
    h.runs[0].bold = True

    sub = doc.add_paragraph("ATOMiK  ·  State-Aware Compute Architecture  ·  Pre-Seed  ·  May 2026")
    sub.runs[0].font.color.rgb = RGBColor(0x22, 0xD3, 0xEE)
    sub.runs[0].font.size = Pt(10)

    doc.add_paragraph()
    return doc


def h1(doc, text):
    p = doc.add_heading(text, 1)
    p.runs[0].font.color.rgb = RGBColor(0x07, 0x0B, 0x12)
    return p


def h2(doc, text):
    p = doc.add_heading(text, 2)
    p.runs[0].font.color.rgb = RGBColor(0x1D, 0x32, 0x4A)
    return p


def body(doc, text, bold=False, size=11):
    p = doc.add_paragraph(text)
    if bold:
        for run in p.runs:
            run.bold = True
    p.runs[0].font.size = Pt(size)
    return p


def bullet(doc, text, color=None):
    p = doc.add_paragraph(text, style="List Bullet")
    p.runs[0].font.size = Pt(10.5)
    if color:
        p.runs[0].font.color.rgb = color
    return p


def table_2col(doc, rows, header=None, widths=(2.8, 4.0)):
    t = doc.add_table(rows=len(rows) + (1 if header else 0), cols=2)
    t.style = "Table Grid"
    if header:
        for i, h in enumerate(header):
            cell = t.rows[0].cells[i]
            cell.text = h
            cell.paragraphs[0].runs[0].bold = True
            cell.paragraphs[0].runs[0].font.size = Pt(9.5)
    offset = 1 if header else 0
    for ri, row in enumerate(rows):
        for ci, val in enumerate(row):
            cell = t.rows[ri + offset].cells[ci]
            cell.text = str(val)
            cell.paragraphs[0].runs[0].font.size = Pt(9.5)
    for col_idx, w in enumerate(widths):
        for row in t.rows:
            row.cells[col_idx].width = Inches(w)
    doc.add_paragraph()


# ─── EXECUTIVE SUMMARY ───────────────────────────────────────────────────────

def gen_executive_summary():
    doc = styled_doc("Executive Summary")
    h1(doc, "The Opportunity")
    body(doc, "ATOMiK is a state-aware compute architecture that reduces wasted state movement in constrained edge, embedded, and AI systems. Every constrained device — from edge sensors to autonomous systems — spends resources rediscovering what hasn't changed. ATOMiK tracks meaningful change instead of moving, scanning, or rebuilding full state.")
    body(doc, "The company is raising $2.0M in pre-seed capital to fund measured customer-value proof, IP strengthening, and ASIC feasibility — positioning ATOMiK for IP licensing or strategic acquisition by a major semiconductor or platform company.")

    h1(doc, "The Problem")
    body(doc, "Constrained systems everywhere pay a hidden tax on unchanged state:")
    bullet(doc, "Data-center energy: 415 TWh globally (2024) → projected 945 TWh by 2030 (IEA)")
    bullet(doc, "U.S. data centers alone: 176 TWh (2023) → 325–580 TWh projected by 2028 (LBNL)")
    bullet(doc, "Edge devices burn battery, generate heat, and use bandwidth scanning state they already know")
    bullet(doc, "AI context systems move full state even when only a small delta matters")
    body(doc, "The root cause: computing has been architected around full-state movement. ATOMiK changes the unit of compute from state to change.", bold=True)

    h1(doc, "The Solution")
    body(doc, "ATOMiK's core primitive: state = reference_state XOR accumulated_delta")
    body(doc, "LOAD a reference. ACCUM only meaningful changes. READ only when reconstruction is needed. SWAP at clean epoch boundaries.")
    body(doc, "The math — XOR algebra — is self-inverse, commutative, and parallelizable. It maps efficiently to hardware. It is not ML, not new silicon; it is a better structural approach to what computing already does with state.")

    h1(doc, "Validated Proof")
    table_2col(doc, [
        ("ATOMiK Desk v0.39-K UI", "HARDWARE_VALIDATED — prototype running on live Zynq FPGA (May 2026)"),
        ("16/16 algebraic property tests", "HARDWARE_VALIDATED — XOR self-inverse, identity, commutativity confirmed on silicon"),
        ("Linux userspace to FPGA path", "HARDWARE_VALIDATED — full OS-to-accelerator path verified"),
        ("AX7020 board performance matrix", "LIVE_MEASURED — 4-way baseline vs. ATOMiK comparison with raw artifacts"),
        ("Formal proof work", "SOFTWARE_VALIDATED — algebraic properties proved in Lean4"),
    ], header=["Proof Item", "Status & Evidence"], widths=(2.2, 4.6))
    body(doc, "Note: Battery, thermal, cooling, water, and footprint outcomes are evaluation targets — they require workload-specific measurement per customer environment. The mechanism has been validated; the outcomes need customer workloads.", size=9)

    h1(doc, "Business Model & Path to Return")
    body(doc, "ATOMiK does not need to outspend incumbents. The path:")
    table_2col(doc, [
        ("Near term (0–12 mo)", "Paid technical evaluations — customer brings workload, baseline, constraint; ATOMiK measures fit"),
        ("Medium term (12–24 mo)", "Design-partner agreements, IP licensing deals with embedded/edge platform companies"),
        ("Long term (18–36 mo)", "Strategic licensing or acquisition by chip/platform company — Qualcomm, NXP, TI, Renesas, ARM, or hyperscaler edge division"),
    ], header=["Timeline", "Activity"], widths=(1.8, 5.0))
    body(doc, "Comparable IP architecture acquisitions in semiconductor history: $50M–$400M+ for validated compute primitives. ATOMiK's path: workload proof → IP diligence → strategic exit.", bold=True)

    h1(doc, "The Ask")
    table_2col(doc, [
        ("Target raise", "$2.0M pre-seed"),
        ("Minimum close", "$1.25M (12 months)"),
        ("Stretch plan", "$2.75M (18+ months)"),
        ("Instrument", "Post-money SAFE (terms require CFO/counsel review)"),
        ("Engineering & demo", "$600K — Zynq hardening, measurement tooling"),
        ("Customer proof", "$400K — workload baselines, evaluation SOWs, artifacts"),
        ("IP / legal", "$300K — patent conversion, diligence packet"),
        ("ASIC feasibility", "$300K — mentor review, go/no-go path"),
        ("Finance / ops", "$250K + $150K reserve"),
        ("Round funds", "Customer proof + IP + ASIC feasibility. NOT tape-out."),
    ], widths=(2.2, 4.6))

    h1(doc, "Contact")
    body(doc, "Matthew Rockwell — Founder\nmatthew.h.rockwell@gmail.com")
    body(doc, "ATOMiK  |  Hardware-validated  |  Pre-seed open  |  May 2026", size=9)

    path = ROOT / "01_pitch_materials" / "ATOMiK_Executive_Summary.docx"
    doc.save(path)
    print(f"✓ {path.name}")


# ─── BUSINESS PLAN ───────────────────────────────────────────────────────────

def gen_business_plan():
    doc = styled_doc("ATOMiK Business Plan")
    h1(doc, "Company Overview")
    body(doc, "ATOMiK is a state-aware compute architecture startup developing a hardware/IP primitive that reduces wasted state movement in constrained edge, embedded, and AI systems. The company is pre-revenue, pre-seed stage, hardware-validated on FPGA, and raising $2.0M to fund measured customer proof and IP diligence.")
    body(doc, "Incorporation status: LLC/C-Corp formation in progress. See data room for current entity documentation.")

    h1(doc, "Problem")
    body(doc, "Every constrained system in the world — edge devices, embedded controllers, AI inference systems, remote industrial equipment — pays a hidden tax on state it hasn't changed. The system scans it, compares it, copies it, moves it, syncs it. Every tick. Even when nothing meaningful changed.")
    body(doc, "This waste manifests as:")
    bullet(doc, "Shortened battery life in edge and IoT devices")
    bullet(doc, "Unnecessary heat in sealed or fanless systems")
    bullet(doc, "Wasted bandwidth over intermittent or expensive links")
    bullet(doc, "Latency from rebuilding or replaying state that could have been tracked")
    bullet(doc, "Hardware overbuild to handle peak-state throughput that is mostly redundant")
    body(doc, "Market scale: Data-center energy 415→945 TWh by 2030 (IEA). U.S. data centers 176→325–580 TWh by 2028 (LBNL). 66B liters direct cooling water (2023 U.S.). Semiconductor market $772B (2025).", size=9)

    h1(doc, "Solution: ATOMiK Architecture")
    body(doc, "Core equation: state = reference_state XOR accumulated_delta")
    body(doc, "Operations: LOAD (set reference state) → ACCUM (accumulate deltas via XOR) → READ (reconstruct current state) → SWAP (commit epoch boundary)")
    body(doc, "The primitive is self-inverse (applying a delta twice returns to original state), commutative (order doesn't matter), associative (can be parallelized), and identity-preserving (XOR with zero is a no-op). These algebraic properties have been formally proven and hardware-validated.")
    body(doc, "What this means for customers: instead of moving, scanning, syncing, or rebuilding full state — move only what changed. Track only meaningful change. Reduce redundant state movement at the source.")

    h1(doc, "Current Proof Status")
    body(doc, "Hardware-validated proof exists. The following is accurate as of May 2026:")
    bullet(doc, "ATOMiK Desk v0.39-K: framebuffer-native prototype UI running on live Zynq FPGA (HARDWARE_VALIDATED)")
    bullet(doc, "Algebraic tests: 16/16 PASS on XC7Z020 hardware through Linux userspace to FPGA (HARDWARE_VALIDATED)")
    bullet(doc, "AX7020 performance matrix: raw board-run artifacts with 4-way comparison (LIVE_MEASURED with documented caveats)")
    bullet(doc, "Formal proofs: algebraic properties proved in Lean4 (SOFTWARE_VALIDATED)")
    bullet(doc, "SD boot artifacts: BOOT.bin and bitstream exist; standalone SD boot in final validation (BUILD_ARTIFACT)")
    body(doc, "IMPORTANT: Battery, heat, cooling, water, and footprint outcomes require workload-specific customer measurement. These are evaluation targets, not current claims.", bold=True, size=9)

    h1(doc, "Target Market & Customer Segments")
    table_2col(doc, [
        ("Edge / embedded", "Battery pressure, enclosure heat, intermittent links, reliability. Metrics: bytes moved, latency, bandwidth, power proxy."),
        ("AI at the edge", "Context/state movement, memory pressure, response time. Metrics: context retained, transfer avoided."),
        ("Remote/industrial/robotics/defense", "Weight, wattage, packet budget, field runtime. Metrics: runtime proxy, update cost."),
        ("Data center / infrastructure", "Power bill, cooling, water pressure, rack density. Metrics: bytes moved, power/thermal path. (Strategic expansion)"),
    ], header=["Segment", "Pain & Target Metrics"], widths=(2.0, 4.8))

    h1(doc, "Business Model")
    body(doc, "ATOMiK is an IP company, not a product company. Revenue path:")
    bullet(doc, "Paid technical evaluations — bring one workload, one baseline, one constraint; receive measured fit assessment")
    bullet(doc, "Design-partner engagements — deeper technical partnership with IP access")
    bullet(doc, "IP licensing — architecture and implementation licensed to chip/platform companies")
    bullet(doc, "Strategic acquisition — acquired by semiconductor or platform company needing this for their edge stack")
    body(doc, "No revenue forecast until signed customer agreements. CFO approval required before projections are shared externally.")

    h1(doc, "Go-to-Market")
    body(doc, "First sale: one constrained team with one state-heavy workload hitting a hard limit. Entry point: technical evaluation. Proof-gate: measured improvement on one agreed metric while preserving correctness.")
    body(doc, "Not: outspend incumbents. Not: build a consumer product. Not: chase the data-center market before edge proof exists.")

    h1(doc, "Competitive Positioning")
    body(doc, "ATOMiK is not a CPU, GPU, NPU, or accelerator replacement. It is a state-aware architecture layer for workloads where state movement is the binding constraint.")
    table_2col(doc, [
        ("Von Neumann CPUs", "Process full state every cycle. ATOMiK: process change."),
        ("Compression", "Reduce stored size. ATOMiK: reduce movement frequency."),
        ("Caching", "Cache hot data. ATOMiK: accumulate delta; avoid unnecessary reads."),
        ("Custom ASICs", "Optimize known workload. ATOMiK: architecture-level primitive applicable across workloads."),
    ], header=["Comparison", "Differentiation"], widths=(1.8, 5.0))

    h1(doc, "IP & Legal")
    body(doc, "Provisional patent filed (see data room: 03_intellectual_property). Patent conversion and prior-art review funded in pre-seed budget. Trade secret protections documented. Pre-seed allocates $300K to IP/legal.")

    h1(doc, "Team")
    body(doc, "Matthew Rockwell — Founder. Hardware engineer, FPGA/ASIC development background. Built the ATOMiK architecture, validated on physical hardware. See data room for full founder profile.")
    body(doc, "Fractional CFO: needed before SAFE terms finalized. First technical hire and advisory board in progress.")

    h1(doc, "Financial Summary")
    body(doc, "See ATOMiK_Financial_Model.xlsx for full detail. Summary:")
    table_2col(doc, [
        ("Pre-seed target", "$2.0M (minimum $1.25M, stretch $2.75M)"),
        ("Instrument", "Post-money SAFE — final terms require CFO/counsel"),
        ("Runway", "18 months at $2.0M raise"),
        ("Engineering", "$600K (demo hardening, measurement tooling)"),
        ("Customer proof", "$400K (evaluation SOWs, baseline harnesses)"),
        ("IP / legal", "$300K (patent conversion, diligence packet)"),
        ("ASIC feasibility", "$300K (expert review, go/no-go)"),
        ("Finance / GTM", "$250K + $150K reserve"),
        ("Tape-out", "NOT funded in this round"),
        ("Revenue", "No forecast without signed agreements"),
    ], widths=(2.2, 4.6))

    h1(doc, "Risks & Mitigations")
    table_2col(doc, [
        ("Customer validation pending", "First evaluation conversation in progress. Budget dedicated to proof."),
        ("Battery/thermal/water outcomes unproven", "Evaluation targets. Require workload measurement. Not claimed yet."),
        ("ASIC economics unvalidated", "Funded in budget. Expert review before tape-out commitment."),
        ("IP protection coverage", "Funded conversion. Prior-art review in progress."),
        ("Incumbent response", "Build IP and proof quickly. Strategic conversations now."),
    ], header=["Risk", "Mitigation"], widths=(2.4, 4.4))

    path = ROOT / "01_pitch_materials" / "ATOMiK_Business_Plan.docx"
    doc.save(path)
    print(f"✓ {path.name}")


# ─── ONE PAGER ───────────────────────────────────────────────────────────────

def gen_one_pager():
    doc = styled_doc("ATOMiK — One Page Overview")
    body(doc, "State-aware compute architecture that reduces wasted state movement in constrained edge, embedded, and AI systems.", bold=True, size=13)
    doc.add_paragraph()

    h2(doc, "The Problem")
    body(doc, "Constrained systems repeatedly move, scan, sync, and rebuild state that hasn't meaningfully changed — burning battery, generating heat, consuming bandwidth, and adding latency. The root cause is architectural: computing moves full state when it could move only change.")

    h2(doc, "The Solution")
    body(doc, "state = reference_state XOR accumulated_delta")
    body(doc, "LOAD a reference → ACCUM meaningful changes → READ only when needed → SWAP at clean boundaries. XOR is compact, self-inverse, and parallelizable. Validated algebraic properties. Hardware-proven on FPGA.")

    h2(doc, "Validated Proof Today")
    table_2col(doc, [
        ("v0.39-K Desk UI on Zynq", "HARDWARE_VALIDATED"),
        ("16/16 algebraic tests on hardware", "HARDWARE_VALIDATED"),
        ("Linux userspace to FPGA path", "HARDWARE_VALIDATED"),
        ("AX7020 board run matrix", "LIVE_MEASURED"),
        ("Formal algebra proofs", "SOFTWARE_VALIDATED"),
    ], widths=(3.5, 3.3))

    h2(doc, "Evaluation Offer")
    body(doc, "Bring one state-heavy workload, your current baseline, and the constraint that already hurts. We evaluate where state movement creates waste. You receive: workload map, baseline comparison, evidence map, fit/no-fit recommendation, and next-step plan.")

    h2(doc, "Business Path")
    body(doc, "Paid evaluations → Design partners → IP licensing → Strategic acquisition by chip/platform company")

    h2(doc, "Pre-Seed Ask: $2.0M")
    table_2col(doc, [
        ("Engineering", "$600K"), ("Customer proof", "$400K"),
        ("IP / legal", "$300K"), ("ASIC feasibility", "$300K"),
        ("Finance / GTM / reserve", "$400K"), ("Tape-out", "NOT in this round"),
    ], widths=(2.5, 4.3))
    body(doc, "Minimum viable close $1.25M. Stretch $2.75M. Post-money SAFE. Final terms require CFO/counsel approval.", size=9)

    body(doc, "\nmatthew.h.rockwell@gmail.com  ·  matthew-rockwell.com  ·  May 2026", size=9)

    path = ROOT / "01_pitch_materials" / "ATOMiK_One_Pager.docx"
    doc.save(path)
    print(f"✓ {path.name}")


# ─── FINANCIAL MODEL (EXCEL) ─────────────────────────────────────────────────

def gen_financial_model():
    wb = openpyxl.Workbook()

    DARK = "070B12"; CYAN = "22D3EE"; GREEN = "22C55E"; AMBER = "F59E0B"; MUTED = "9FB1C7"
    LIGHT = "F4F8FF"; PANEL = "0D1420"

    def hdr(ws, row, col, text, bold=True, bg=DARK, fg=CYAN, sz=11):
        c = ws.cell(row, col, text)
        c.font = Font(name="Calibri", bold=bold, color=fg, size=sz)
        c.fill = PatternFill("solid", fgColor=bg)
        c.alignment = Alignment(horizontal="center", vertical="center", wrap_text=True)
        return c

    def val(ws, row, col, v, fmt=None, bold=False, fg="000000", bg=None):
        c = ws.cell(row, col, v)
        c.font = Font(name="Calibri", bold=bold, color=fg, size=10)
        if bg: c.fill = PatternFill("solid", fgColor=bg)
        if fmt: c.number_format = fmt
        c.alignment = Alignment(horizontal="right" if isinstance(v, (int, float)) else "left")
        return c

    # ── Sheet 1: Use of Funds ──────────────────────────────────────────────
    ws = wb.active; ws.title = "Use of Funds"
    ws.column_dimensions['A'].width = 32
    for col in 'BCDE': ws.column_dimensions[col].width = 16

    hdr(ws, 1, 1, "ATOMiK — Pre-Seed Use of Funds", fg=CYAN, bg=DARK, sz=14)
    ws.merge_cells('A1:E1')
    hdr(ws, 2, 1, "Category", fg=LIGHT); hdr(ws, 2, 2, "Minimum $1.25M"); hdr(ws, 2, 3, "Target $2.0M"); hdr(ws, 2, 4, "Stretch $2.75M"); hdr(ws, 2, 5, "Purpose")
    rows = [
        ("Engineering & Demo", 375_000, 600_000, 825_000, "Zynq hardening, measurement tooling, first technical capacity"),
        ("Customer Proof", 250_000, 400_000, 550_000, "Workload baselines, evaluation SOWs, measured artifacts"),
        ("IP / Legal", 187_500, 300_000, 412_500, "Patent conversion, prior-art review, diligence packet"),
        ("ASIC Feasibility", 187_500, 300_000, 412_500, "Mentor review, feasibility study, quoted tape-out path"),
        ("Finance / GTM / Ops", 156_250, 250_000, 343_750, "CFO, investor reporting, design-partner outreach, ops"),
        ("Reserve", 93_750, 150_000, 206_250, "Runway buffer and unforeseen proof costs"),
    ]
    for i, (name, mn, tgt, sx, purpose) in enumerate(rows):
        r = 3 + i
        bg = "111827" if i % 2 == 0 else "0D1420"
        val(ws, r, 1, name, bold=True, fg=LIGHT, bg=bg)
        val(ws, r, 2, mn, '"$"#,##0', fg=GREEN, bg=bg)
        val(ws, r, 3, tgt, '"$"#,##0', fg=GREEN, bg=bg)
        val(ws, r, 4, sx, '"$"#,##0', fg=GREEN, bg=bg)
        val(ws, r, 5, purpose, fg=MUTED, bg=bg)
    # Total
    val(ws, 9, 1, "TOTAL", bold=True, fg=CYAN, bg=DARK)
    for col, total in [(2, 1_250_000), (3, 2_000_000), (4, 2_750_000)]:
        c = ws.cell(9, col, total); c.font = Font(bold=True, color=CYAN, size=11)
        c.number_format = '"$"#,##0'; c.fill = PatternFill("solid", fgColor=DARK)
    val(ws, 9, 5, "Not-for-tape-out. Funds proof, IP, feasibility.", fg=AMBER, bg=DARK)

    val(ws, 11, 1, "Instrument: Post-money SAFE (final terms require CFO/counsel)", fg=MUTED, bg=DARK)
    val(ws, 12, 1, "Source: ATOMiK pre-seed financing plan, May 2026", fg=MUTED, bg=DARK)

    # ── Sheet 2: Revenue Scenarios ─────────────────────────────────────────
    ws2 = wb.create_sheet("Revenue Scenarios")
    ws2.column_dimensions['A'].width = 30
    for col in 'BCDEF': ws2.column_dimensions[col].width = 16

    hdr(ws2, 1, 1, "ATOMiK — Revenue Scenarios (INTERNAL — NOT FOR EXTERNAL DISTRIBUTION)", fg=AMBER, bg=DARK, sz=12)
    ws2.merge_cells('A1:F1')
    hdr(ws2, 2, 1, "Note", fg=AMBER, bg=DARK)
    ws2.merge_cells('A2:F2')
    ws2.cell(2, 1).value = "These are planning scenarios only. No revenue is forecasted without signed customer agreements. CFO review required."

    hdr(ws2, 4, 1, "Revenue Stream"); hdr(ws2, 4, 2, "Q3 2026"); hdr(ws2, 4, 3, "Q4 2026"); hdr(ws2, 4, 4, "H1 2027"); hdr(ws2, 4, 5, "H2 2027"); hdr(ws2, 4, 6, "Notes")
    streams = [
        ("Paid technical evaluations", 0, "TBD", "TBD", "TBD", "Requires first signed SOW"),
        ("Sponsored proof work", 0, 0, "TBD", "TBD", "Requires design partner agreement"),
        ("IP licensing", 0, 0, 0, "TBD", "Post-diligence milestone"),
        ("Strategic / acquisition", 0, 0, 0, "TBD", "Long-term thesis; not near-term revenue"),
    ]
    for i, row in enumerate(streams):
        r = 5 + i
        bg = "111827" if i % 2 == 0 else "0D1420"
        for ci, v in enumerate(row):
            c = ws2.cell(r, ci + 1, v)
            c.font = Font(name="Calibri", color=GREEN if isinstance(v, int) else LIGHT, size=10)
            c.fill = PatternFill("solid", fgColor=bg)
            if isinstance(v, int): c.number_format = '"$"#,##0'

    # ── Sheet 3: Market Context ────────────────────────────────────────────
    ws3 = wb.create_sheet("Market Context")
    ws3.column_dimensions['A'].width = 38
    for col in 'BCD': ws3.column_dimensions[col].width = 20

    hdr(ws3, 1, 1, "Market Context — Source-Backed Reference Data", fg=CYAN, bg=DARK, sz=13)
    ws3.merge_cells('A1:D1')
    hdr(ws3, 2, 1, "Metric"); hdr(ws3, 2, 2, "Current"); hdr(ws3, 2, 3, "Projected"); hdr(ws3, 2, 4, "Source")
    market_data = [
        ("Global data-center energy", "415 TWh (2024)", "945 TWh (2030)", "IEA Energy and AI Report"),
        ("U.S. data-center energy", "176 TWh (2023)", "325–580 TWh (2028)", "LBNL 2024 U.S. Data Center Report"),
        ("U.S. data-center cooling water", "66B liters (2023)", "60–124B liters (2028)", "LBNL 2024"),
        ("Global semiconductor market", "$772.2B (2025)", "$975.4B (2026)", "SIA / WSTS"),
        ("U.S. pre-seed deals (Q1 2026)", "~3,000 deals / $2.3B raised", "—", "Carta Q1 2026"),
        ("Seed pre-money median", "$18.4M (Q1 2026)", "—", "PitchBook/NVCA Q1 2026 (benchmark only — not ATOMiK valuation)"),
        ("Seed deal size median", "$3.0M (Q1 2026)", "—", "PitchBook/NVCA Q1 2026"),
        ("YC standard check", "$500K for 7%", "—", "YCombinator standard terms"),
    ]
    for i, row in enumerate(market_data):
        r = 3 + i
        bg = "111827" if i % 2 == 0 else "0D1420"
        for ci, v in enumerate(row):
            c = ws3.cell(r, ci + 1, v)
            c.font = Font(name="Calibri", color=LIGHT, size=10)
            c.fill = PatternFill("solid", fgColor=bg)
    val(ws3, 12, 1, "Market framing only. Not ATOMiK-specific savings claims. All figures from cited sources.", fg=AMBER, bg=DARK)

    # ── Sheet 4: Cap Table ─────────────────────────────────────────────────
    ws4 = wb.create_sheet("Cap Table (Draft)")
    ws4.column_dimensions['A'].width = 30
    for col in 'BCDE': ws4.column_dimensions[col].width = 16

    hdr(ws4, 1, 1, "Cap Table — DRAFT / PLACEHOLDER (CFO Review Required)", fg=AMBER, bg=DARK, sz=12)
    ws4.merge_cells('A1:E1')
    hdr(ws4, 3, 1, "Holder"); hdr(ws4, 3, 2, "Share Class"); hdr(ws4, 3, 3, "Shares"); hdr(ws4, 3, 4, "% Pre-Money"); hdr(ws4, 3, 5, "Notes")
    cap_rows = [
        ("Matthew Rockwell (Founder)", "Common", "TBD", "~100%", "Pre-money; exact count pending incorporation"),
        ("ESOP / Option Pool", "Common (Reserved)", "TBD", "0% (to be created)", "Recommend 10–15% pool at first close"),
        ("Pre-Seed Investors", "SAFE", "TBD", "Post-money via SAFE", "Converts at next priced round"),
    ]
    for i, row in enumerate(cap_rows):
        r = 4 + i
        bg = "111827" if i % 2 == 0 else "0D1420"
        for ci, v in enumerate(row):
            c = ws4.cell(r, ci + 1, v)
            c.font = Font(name="Calibri", color=LIGHT if ci < 4 else MUTED, size=10)
            c.fill = PatternFill("solid", fgColor=bg)

    val(ws4, 8, 1, "IMPORTANT: This is a placeholder. Actual cap table requires:", fg=AMBER, bg=DARK)
    val(ws4, 9, 1, "• Articles of incorporation and stock issuance", fg=MUTED, bg=DARK)
    val(ws4, 10, 1, "• CFO/counsel review and approval", fg=MUTED, bg=DARK)
    val(ws4, 11, 1, "• Capitalization software (e.g., Carta) for tracking", fg=MUTED, bg=DARK)
    val(ws4, 12, 1, "Do not share externally until reviewed by counsel.", fg=AMBER, bg=DARK)

    path = ROOT / "02_financial" / "ATOMiK_Financial_Model.xlsx"
    wb.save(path)
    print(f"✓ {path.name}")


# ─── LEGAL & FORMATION DOC ───────────────────────────────────────────────────

def gen_legal_summary():
    doc = styled_doc("ATOMiK — Legal & Formation Summary")
    body(doc, "CONFIDENTIAL — DATA ROOM DOCUMENT — NOT FOR PUBLIC DISTRIBUTION", bold=True)
    body(doc, "Last updated: May 2026. Requires counsel review before investor distribution.", size=9)
    doc.add_paragraph()

    h1(doc, "Entity Status")
    body(doc, "Formation status: In progress. See entity_status.md in data room for current filing status.")
    body(doc, "Recommended structure: Delaware C-Corporation (standard for VC-backed companies).")
    body(doc, "⚠️ REQUIRED BEFORE CLOSE: Articles of incorporation, EIN, bank account, stock issuance to founder, SAFE template reviewed by counsel.")

    h1(doc, "Intellectual Property")
    body(doc, "Provisional patent filed — see data room 03_intellectual_property for filing details.")
    body(doc, "Patent conversion funded in pre-seed budget ($300K IP/legal allocation).")
    body(doc, "Trade secret documentation in progress (see trade_secrets.md in data room).")
    body(doc, "IP assignment: Founder IP assignment agreement required before close. Template in data room.")

    h1(doc, "Capitalization")
    body(doc, "See ATOMiK_Financial_Model.xlsx → Cap Table (Draft) sheet.")
    body(doc, "⚠️ Formal cap table requires incorporation + Carta setup + counsel review.")
    body(doc, "Pre-seed instrument: Post-money SAFE. Valuation cap, discount, and pro-rata rights require CFO/counsel approval.")

    h1(doc, "Employment & Contractors")
    body(doc, "Current: Founder only. No employees. No contractor agreements active.")
    body(doc, "Recommended before close: Formal employment agreement for founder, consulting agreements for any contractors, IP assignment for all contributors.")

    h1(doc, "Data Room Checklist for Investor Due Diligence")
    items = [
        ("Articles of incorporation / formation docs", "PENDING"), ("EIN / Federal Tax ID", "PENDING"),
        ("Founder stock issuance documents", "PENDING"), ("Cap table (Carta or equivalent)", "PENDING"),
        ("IP assignment agreements", "TEMPLATE READY"), ("Provisional patent filing", "✓ FILED"),
        ("Patent conversion plan", "IN PROGRESS"), ("SAFE template (reviewed by counsel)", "PENDING CFO/COUNSEL"),
        ("Employment agreements", "PENDING"), ("Contractor agreements", "NONE ACTIVE"),
        ("Board resolution (if applicable)", "PENDING"), ("Prior IP clearance", "IN PROGRESS"),
    ]
    table_2col(doc, items, header=["Document", "Status"], widths=(3.0, 3.8))

    path = ROOT / "03_data_room" / "legal" / "ATOMiK_Legal_Formation_Summary.docx"
    doc.save(path)
    print(f"✓ {path.name}")


# ─── PRODUCT / TECH DOC ──────────────────────────────────────────────────────

def gen_tech_doc():
    doc = styled_doc("ATOMiK — Product & Technical Overview")
    body(doc, "DATA ROOM — TECHNICAL SUMMARY", bold=True, size=9)

    h1(doc, "Architecture Overview")
    body(doc, "ATOMiK is a state-aware compute primitive implemented as hardware IP (FPGA-validated) with a path to ASIC. The core equation: state = reference_state XOR accumulated_delta")
    body(doc, "Operations:")
    table_2col(doc, [
        ("LOAD(addr, initial_state)", "Write reference state to address slot"),
        ("ACCUM(delta)", "XOR a delta into the current accumulator"),
        ("READ()", "Reconstruct current state: reference XOR accumulated deltas"),
        ("SWAP(addr)", "Commit clean epoch boundary"),
    ], widths=(2.5, 4.3))

    h1(doc, "Hardware Implementation")
    table_2col(doc, [
        ("Platform", "Zynq XC7Z020-2CLG484I (HamGeek RK-ZYNQ7020-F)"),
        ("Soft CPU", "NaxRiscv RV64GC (production) / VexRiscv SMP rv32ima (baseline validation)"),
        ("ATOMiK core", "Single-bank, 256×64-bit state table, LiteX Migen CSR"),
        ("CSR base", "0xf0020000 (NaxRiscv SoC)"),
        ("Linux", "Ubuntu 24.04 / Buildroot 6.9, userspace access via /dev/mem"),
        ("Bitstream", "hamgeek_rk7020f.bit (LiteX synthesis, Vivado 2025.2)"),
        ("FPGA utilization", "~63% LUT for full NaxRiscv RV64 + ATOMiK + PSUart0Bridge"),
    ], widths=(2.0, 4.8))

    h1(doc, "Validated Proof")
    table_2col(doc, [
        ("16/16 algebraic property tests", "HARDWARE_VALIDATED — PASS on XC7Z020 through Linux userspace"),
        ("ATOMiK Desk v0.39-K UI", "HARDWARE_VALIDATED — framebuffer-native prototype on live hardware"),
        ("AX7020 board run matrix", "LIVE_MEASURED — 4-way comparison with raw artifacts"),
        ("Formal proofs (Lean4)", "SOFTWARE_VALIDATED — algebraic properties formally proved"),
        ("SD boot artifacts", "BUILD_ARTIFACT — BOOT.bin and bitstream exist; final validation in progress"),
    ], widths=(2.5, 4.3))

    h1(doc, "MMIO Interface")
    body(doc, "Accessible from Linux userspace via /dev/mem at CSR base 0xf0020000. Ordering requirement: after CSR writes, issue fence iorw,iorw + dummy STATUS read before reading STATE_LO/HI to flush Wishbone pipeline.")

    h1(doc, "ASIC Path")
    body(doc, "The architecture is ASIC-portable. XOR is 1-2 gates per bit in standard cell. The 256×64-bit state table is 16KB — manageable in any modern node.")
    body(doc, "Pre-seed allocates $300K for ASIC feasibility (mentor-reviewed go/no-go). Tape-out is NOT in this round.")

    h1(doc, "Product Roadmap (High-Level)")
    table_2col(doc, [
        ("Now", "Zynq FPGA prototype validated. Desktop UI. Algebraic proof. Workload harness ready."),
        ("Q3 2026", "P0 workloads measured on hardware. First paid evaluation. SD boot stable."),
        ("Q4 2026", "IP diligence packet. Design partner agreement(s). ASIC feasibility study."),
        ("2027", "IP licensing or strategic partnership. ASIC feasibility outcome determines path."),
    ], widths=(1.5, 5.3))

    h1(doc, "Source Code & Artifacts")
    body(doc, "Private GitHub repository. Access granted under NDA.")
    bullet(doc, "hardware/rtl/ — ATOMiK core RTL (atomik_core_v2.v, delta_acc, state_rec)")
    bullet(doc, "hardware/zynq/ — Zynq SoC, FSBL, NaxRiscv integration")
    bullet(doc, "software/atomik_sdk/ — Python SDK, 353 tests, formal proof integration")
    bullet(doc, "math/proofs/ — Lean4 formal proofs of algebraic properties")
    bullet(doc, "results/ — Raw measurement artifacts with evidence labels")

    path = ROOT / "03_data_room" / "product_tech" / "ATOMiK_Product_Technical_Overview.docx"
    doc.save(path)
    print(f"✓ {path.name}")


# ─── CUSTOMER PIPELINE ───────────────────────────────────────────────────────

def gen_customer_pipeline():
    doc = styled_doc("ATOMiK — Customer & Sales Pipeline")
    body(doc, "CONFIDENTIAL DATA ROOM DOCUMENT", bold=True, size=9)

    h1(doc, "Current Status")
    body(doc, "ATOMiK is pre-revenue. No signed customer contracts or LOIs exist as of May 2026. This document describes the pipeline strategy and evaluation offer structure.", bold=True)

    h1(doc, "Evaluation Offer")
    body(doc, "Standard entry point: Bring one state-heavy workload, your current baseline, and the constraint that already hurts.")
    body(doc, "ATOMiK delivers: workload map, baseline comparison, evidence map, fit/no-fit recommendation, and next-step plan.")
    body(doc, "This can be structured as: (a) unpaid feasibility conversation, (b) paid technical evaluation (SOW-based), or (c) design-partner engagement with IP access.")

    h1(doc, "Target Segment Priority")
    table_2col(doc, [
        ("Edge / embedded OEMs", "Battery, heat, bandwidth-constrained products (IoT, industrial, automotive edge)"),
        ("Defense / remote systems", "Size, weight, power (SWaP) constrained; packet budget; field runtime"),
        ("AI at the edge", "Context movement, memory pressure, local inference latency"),
        ("Data center / infrastructure", "Strategic expansion path — requires edge proof first"),
    ], header=["Segment", "Pain & Entry Point"], widths=(2.2, 4.6))

    h1(doc, "Active Outreach")
    body(doc, "See outreach/ directory in repo for current status of VC and design-partner conversations.")
    body(doc, "Previous investor conversations: Chris Bolt (notes in vc_diligence_response_chris_bolt.md), Jeremy Mueller (meeting_prep_jeremy_mueller.md).")

    h1(doc, "LOI & Contract Templates")
    body(doc, "Paid evaluation proposal template: business/design_partners/paid_evaluation_proposal.md")
    body(doc, "LOI outline: business/design_partners/loi_outline.md")
    body(doc, "⚠️ No signed LOIs or customer contracts exist as of this writing. First signed agreement is a key pre-seed milestone.")

    h1(doc, "Pipeline Gate")
    body(doc, "The $400K customer proof budget funds the first 1–3 paid evaluations. Success metric: measured improvement on one agreed metric while preserving correctness, with raw artifacts and claims registry entry.")

    path = ROOT / "03_data_room" / "customers" / "ATOMiK_Customer_Pipeline.docx"
    doc.save(path)
    print(f"✓ {path.name}")


# ─── TEAM DOC ────────────────────────────────────────────────────────────────

def gen_team_doc():
    doc = styled_doc("ATOMiK — Team Overview")
    h1(doc, "Founder")
    body(doc, "Matthew H. Rockwell — Founder & CEO", bold=True, size=14)
    body(doc, "Email: matthew.h.rockwell@gmail.com")
    body(doc, "Role: Architecture, hardware development, software, business development. All ATOMiK proof artifacts and hardware implementation.")
    body(doc, "Background: FPGA/ASIC hardware engineering, embedded systems, state-machine architecture. Built and validated ATOMiK on Zynq FPGA hardware. Full-stack from RTL to Linux userspace.")
    body(doc, "See founder_profile.md in data room for extended profile.", size=9)

    h1(doc, "Planned First Hires (Post-Close)")
    table_2col(doc, [
        ("Senior Hardware Engineer", "ASIC-capable; RTL optimization, place-and-route, customer workload harness"),
        ("Customer Engineer / Solutions", "Technical evaluation lead; customer workload proof execution"),
        ("Fractional CFO", "SAFE terms, investor reporting, financial model, board prep"),
    ], header=["Role", "Focus"], widths=(2.5, 4.3))
    body(doc, "See first_hires.md in data room for detailed requirements.", size=9)

    h1(doc, "Advisory Board (In Progress)")
    body(doc, "Target advisors: semiconductor IP expert, ASIC/tape-out mentor, enterprise sales advisor with hardware/IP background.")
    body(doc, "No advisors formally signed as of May 2026. See advisory_board_plan.md for target criteria.", size=9)

    h1(doc, "Gaps Before Raise Close")
    bullet(doc, "Fractional CFO for SAFE terms and financial model")
    bullet(doc, "IP counsel for patent conversion and SAFE review")
    bullet(doc, "ASIC feasibility mentor (funded in pre-seed)")

    path = ROOT / "03_data_room" / "team" / "ATOMiK_Team_Overview.docx"
    doc.save(path)
    print(f"✓ {path.name}")


# ─── COPY FILES ──────────────────────────────────────────────────────────────

def copy_files():
    # Pitch deck PPTX
    src_deck = ROOT.parent / "pitch_deck" / "ATOMiK_Investor_Deck.pptx"
    if src_deck.exists():
        shutil.copy(src_deck, ROOT / "01_pitch_materials" / "ATOMiK_Investor_Deck.pptx")
        print(f"✓ ATOMiK_Investor_Deck.pptx (copied)")

    # Provisional patent PDF
    src_patent = ROOT.parent / "data_room" / "03_intellectual_property" / "Provisional Patent 0.0.1.pdf"
    if src_patent.exists():
        shutil.copy(src_patent, ROOT / "03_data_room" / "product_tech" / "ATOMiK_Provisional_Patent.pdf")
        print(f"✓ ATOMiK_Provisional_Patent.pdf (copied)")

    # Talking points
    src_tp = ROOT.parent / "pitch_deck" / "INVESTOR_TALKING_POINTS_FRIDAY.md"
    if src_tp.exists():
        shutil.copy(src_tp, ROOT / "01_pitch_materials" / "ATOMiK_Talking_Points.md")
        print(f"✓ ATOMiK_Talking_Points.md (copied)")

    # Key source markdown files for Codex reference
    src_docs = [
        ("pitch_deck/slides.md", "04_source_markdown/slides_source.md"),
        ("data_room/01_financial/financial_model.md", "04_source_markdown/financial_model_source.md"),
        ("data_room/01_financial/revenue_model_revised.md", "04_source_markdown/revenue_model_source.md"),
        ("faq/investor_faq.md", "04_source_markdown/investor_faq_source.md"),
        ("one_pager/atomik_one_pager.md", "04_source_markdown/one_pager_source.md"),
    ]
    for src_rel, dst_rel in src_docs:
        src = ROOT.parent / src_rel
        if src.exists():
            shutil.copy(src, ROOT / dst_rel)
            print(f"✓ {dst_rel} (copied)")


# ─── CODEX README ────────────────────────────────────────────────────────────

def gen_codex_readme():
    readme = """# ATOMiK Investor Package — Codex Review Folder

## Purpose
This folder contains all investor-facing documents for the Friday Aggie Angel pitch.
Codex: review each document and make improvements listed below.

## What to Review

### 01_pitch_materials/
- **ATOMiK_Investor_Deck.pptx** — 12-slide deck. Review slides for clarity, flow, VC-friendly language.
  - Ensure every slide gets to the point in <5 seconds of reading
  - Remove any tech jargon a non-engineer VC wouldn't immediately understand
  - Strengthen the ROI/return narrative on slide 9 (Return Path) and slide 12 (The Ask)
  - Verify all numbers match the financial model
- **ATOMiK_Executive_Summary.docx** — 1-2 page overview. Tighten to 1 page if possible.
- **ATOMiK_Business_Plan.docx** — Full business plan. Review for accuracy and VC-friendly tone.
- **ATOMiK_One_Pager.docx** — Single page overview. Must fit on one page. Trim aggressively.
- **ATOMiK_Talking_Points.md** — Pitch script. Review for natural speech, not corporate speak.

### 02_financial/
- **ATOMiK_Financial_Model.xlsx** — Review all 4 sheets:
  - Use of Funds: verify numbers sum correctly
  - Revenue Scenarios: flag any cells that need real data before investor meeting
  - Market Context: verify all source citations are accurate
  - Cap Table: clearly mark as DRAFT and ensure it doesn't create false impressions
- Flag any numbers that need CFO approval before sharing

### 03_data_room/
- **legal/ATOMiK_Legal_Formation_Summary.docx** — Ensure all PENDING items are clearly flagged
- **product_tech/ATOMiK_Product_Technical_Overview.docx** — Make accessible to non-engineers
- **customers/ATOMiK_Customer_Pipeline.docx** — Honest status of pipeline
- **team/ATOMiK_Team_Overview.docx** — Professional, factual

## Codex Instructions

1. **DO NOT** change any financial numbers without flagging them for human review
2. **DO NOT** remove evidence labels (HARDWARE_VALIDATED, LIVE_MEASURED, etc.)
3. **DO NOT** add performance claims that aren't in the claims_registry.yaml
4. **DO** improve prose clarity, grammar, flow, and VC-appropriate language
5. **DO** ensure consistent formatting within each document
6. **DO** strengthen the investor ROI narrative where appropriate
7. **DO** flag any inconsistencies between documents
8. **DO** note any PENDING items that need human action before Friday

## Key Claims Rules (DO NOT VIOLATE)
- Only claim battery/heat/water/cooling savings if there's a measured artifact
- Always pair performance numbers with evidence labels
- Never say "production ready" or "commercial product"
- Always say what IS measured vs what is an evaluation target

## Evidence Labels Currently Active
- HARDWARE_VALIDATED: v0.39-K UI, algebraic tests, Linux userspace path
- LIVE_MEASURED: AX7020 board run matrix
- SOFTWARE_VALIDATED: Formal algebraic proofs
- BUILD_ARTIFACT: SD boot artifacts

## Source Files
- 04_source_markdown/ contains the canonical markdown sources
- Website source is in /website/src/
- Claims registry is in /results/claims_registry.yaml

## Contact
matthew.h.rockwell@gmail.com
"""
    with open(ROOT / "CODEX_README.md", "w") as f:
        f.write(readme)
    print("✓ CODEX_README.md")


# ─── MAIN ────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    print("Generating ATOMiK investor package...\n")
    gen_executive_summary()
    gen_business_plan()
    gen_one_pager()
    gen_financial_model()
    gen_legal_summary()
    gen_tech_doc()
    gen_customer_pipeline()
    gen_team_doc()
    copy_files()
    gen_codex_readme()
    print("\n✓ All documents generated in business/for_codex/")
    print("  Open ATOMiK_Investor_Deck.pptx to verify visual quality.")
    print("  Open ATOMiK_Financial_Model.xlsx to verify numbers.")
    print("  All .docx files ready for Codex review.")
