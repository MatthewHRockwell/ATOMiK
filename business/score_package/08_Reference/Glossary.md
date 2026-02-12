# ATOMiK Glossary

*Technical terms translated to plain English. Reference this whenever you encounter unfamiliar terminology in the ATOMiK materials.*

---

## Core Concepts

**Delta**
A change. Instead of storing the full current state, ATOMiK stores only what changed. Think of it like track changes in a Word document — you see the edits, not a whole new copy of the document.

**Delta-State Algebra**
The mathematical system that governs how ATOMiK processes changes. It defines the rules for combining, undoing, and merging changes — and proves these rules always work correctly.

**State**
The current condition of a piece of data. For example, the current price of a stock, the current reading of a sensor, or the current position of a game character.

**State Reconstruction**
Building the current state from a starting point plus all the changes that have been applied. ATOMiK does this in a single operation (O(1)), regardless of how many changes have accumulated.

**XOR (Exclusive Or)**
A simple math operation that computers can do in one step. It compares two pieces of data bit by bit: if the bits are different, the result is 1; if they're the same, the result is 0. XOR is special because it's its own undo — applying it twice gets you back to where you started.

**Accumulator**
A hardware component that collects and combines changes over time. ATOMiK's accumulator applies each new delta to the running total using XOR. Think of it like a running total on a calculator that updates with each new input.

---

## Mathematical Properties

**Commutative**
Order doesn't matter. A + B gives the same result as B + A. For ATOMiK, this means changes can arrive in any order and the final state is always the same — critical for systems where network delays cause data to arrive out of sequence.

**Associative**
Grouping doesn't matter. (A + B) + C gives the same result as A + (B + C). This means ATOMiK can split work across multiple processors and combine the results — the answer is always correct regardless of how the work was divided.

**Self-Inverse**
Every change is its own undo. Applying the same change twice cancels it out, returning to the original state. This gives ATOMiK free, instant rollback — no need to save backup copies or replay history.

**Identity Element**
A "do nothing" value. Applying the identity element to any state leaves it unchanged. In ATOMiK, this is a delta of all zeros — it represents "no change."

**Formal Proof**
A mathematical argument verified by a computer program, not a human. Unlike testing (which checks specific cases), a formal proof guarantees a statement is true in every possible scenario. ATOMiK has 92 of these, covering all core operations.

**Lean4**
A software tool that mathematically proves statements are true — like a calculator for logic. Lean4 checks every step of a proof automatically, ensuring no errors. It's used by mathematicians, software engineers, and now ATOMiK to verify correctness.

**Theorem**
A mathematical statement that has been proven true. Each of ATOMiK's 92 theorems states a specific property (like "XOR is commutative") and includes a complete proof verified by Lean4.

---

## Hardware Terms

**FPGA (Field-Programmable Gate Array)**
A programmable computer chip — like a blank circuit board you can configure. Unlike a regular processor that runs software instructions, an FPGA is physically wired to perform specific operations. This makes it much faster for specialized tasks. ATOMiK uses a $10 FPGA as its prototype platform.

**ASIC (Application-Specific Integrated Circuit)**
A custom chip designed for one specific purpose. Faster and cheaper per unit than an FPGA, but costs millions of dollars to design and manufacture. ASICs make sense when you need to produce hundreds of thousands or millions of identical chips.

**RTL (Register-Transfer Level)**
The "source code" for a chip. Just as software is written in programming languages, hardware circuits are designed in RTL. ATOMiK's RTL describes the exact circuit that implements delta-state operations.

**Verilog**
A programming language for designing computer chips. ATOMiK's hardware is written in Verilog. It describes the physical circuits, logic gates, and connections that make up the chip.

**LUT (Look-Up Table)**
The basic building block inside an FPGA. Think of it as a small, fast memory that implements a logic function. ATOMiK's 16-bank design uses only 20% of the available LUTs on the $10 chip — meaning there's significant room to grow.

**Tang Nano 9K**
The specific $10 FPGA development board that ATOMiK runs on. Made by Sipeed using a Gowin GW1NR-9 FPGA chip. It's small (about the size of a USB stick), cheap, and powerful enough to demonstrate 1 billion operations per second.

**Clock Cycle**
One "tick" of the chip's internal clock. Everything inside a digital chip happens in sync with its clock. ATOMiK completes one full operation per clock cycle — about 10.6 nanoseconds (billionths of a second) on the current chip.

**Synthesis**
The process of converting RTL code into an actual circuit layout on an FPGA or ASIC. Similar to how a compiler converts software source code into executable programs.

**Carry Chain**
A cascading sequence of calculations inside a chip. Adding two large numbers requires carrying values from one digit to the next — each carry depends on the previous one, creating a chain that slows things down. XOR has no carry chains, which is why ATOMiK is so fast.

**Parallel Banks**
Multiple copies of the ATOMiK processing unit running simultaneously. Each bank processes changes independently, and their results are merged at the end. ATOMiK's 16-bank design processes 16 operations in parallel every clock cycle.

**Merge Tree**
A structure that combines results from multiple parallel banks into a single result. ATOMiK uses a binary merge tree — pairs of banks merge, then pairs of pairs merge, and so on — to efficiently combine all parallel results.

---

## Software & SDK Terms

**SDK (Software Development Kit)**
A set of tools that software developers use to build applications. ATOMiK's SDK generates code in 5 programming languages from a single definition, saving developers from writing the same logic five times.

**Code Generation**
Automatically creating source code from a high-level description. Instead of a developer manually writing code in Python, Rust, C, JavaScript, and Verilog, ATOMiK's SDK generates all five from a single schema definition.

**Schema**
A blueprint that describes the structure of data. In ATOMiK's SDK, a schema defines what data fields exist, their types, and their relationships. The SDK reads this schema and generates code for all target languages.

**API (Application Programming Interface)**
A set of rules that allows different software programs to communicate with each other. ATOMiK's SDK provides APIs that let applications use delta-state operations without knowing the underlying hardware details.

**CI/CD (Continuous Integration / Continuous Deployment)**
Automated systems that test and deploy software every time a change is made. ATOMiK's SDK includes 242 automated tests that run continuously to ensure everything works correctly.

---

## Performance Terms

**Throughput**
How many operations a system can perform per unit of time. ATOMiK's throughput is 1,056 million operations per second (Mops/s) with 16 parallel banks. Higher is better.

**Latency**
How long a single operation takes to complete. ATOMiK's latency is 10.6 nanoseconds per operation. Lower is better.

**Mops/s (Millions of Operations per Second)**
A standard measure of processing speed. ATOMiK achieves 1,056 Mops/s — just over 1 billion operations per second.

**Linear Scaling**
Performance that increases proportionally with added resources. Double the parallel banks = double the throughput. This is the ideal scaling behavior; many systems show diminishing returns as they scale up. ATOMiK's linear scaling is mathematically proven.

**O(1) — Constant Time**
An operation that takes the same amount of time regardless of the size of the data. ATOMiK's state reconstruction is O(1) — whether there have been 10 changes or 10 million changes, reconstruction takes the same single operation.

**O(N) — Linear Time**
An operation that takes longer as the data grows. Traditional event sourcing has O(N) reconstruction — replaying all N events. If you have a million events, you replay a million events. ATOMiK replaces this with O(1).

---

## Business Terms

**IP (Intellectual Property)**
The designs, patents, and know-how that give a company competitive advantage. ATOMiK's IP includes its chip designs, mathematical proofs, and SDK software.

**IP Licensing**
Allowing other companies to use your intellectual property in exchange for payment (fees and/or royalties). This is how ARM makes money — they design processor cores and license the designs to chip manufacturers.

**Royalty**
A per-unit payment made each time a product using licensed IP is manufactured or sold. If ATOMiK's IP is in a chip, ATOMiK receives a small payment for every chip produced.

**NRE (Non-Recurring Engineering)**
The one-time cost to design and set up manufacturing for a new chip. For ASICs, this can be $2-10 million. It's "non-recurring" because you pay it once, then produce chips at low per-unit cost.

**TAM (Total Addressable Market)**
The total revenue opportunity if every potential customer adopted the product. ATOMiK's TAM is $500B+ across all applicable industries.

**SAM (Serviceable Addressable Market)**
The portion of the TAM that ATOMiK can realistically target with its current capabilities. ATOMiK's SAM is approximately $50B, focusing on FPGA-based applications, real-time data infrastructure, IoT, and defense.

**SOM (Serviceable Obtainable Market)**
The realistic revenue ATOMiK can capture in the near term. Estimated at $500M over 5 years, based on penetrating 3-5 key verticals.

**Patent Pending**
ATOMiK has filed patent applications to protect its architecture and methods. "Pending" means the applications are submitted and under review by the patent office. Once granted, patents provide legal exclusivity for 20 years.

**Moat**
A competitive advantage that's difficult for others to replicate. ATOMiK's moat includes patents, 92 formal proofs (years of mathematical work), working hardware, and a full-stack solution from proofs to silicon to software.

---

## Demo-Specific Terms

**Simulation Mode**
Running the ATOMiK demo using software that mimics what the hardware does, without requiring the physical chip. All demo features work in simulation — you don't need the $10 board to see ATOMiK in action.

**TUI (Text-based User Interface)**
A graphical interface that runs inside a terminal/command prompt window, using text characters to draw boxes, charts, and tables. ATOMiK's TUI provides a rich demo experience without needing a web browser.

**Act**
One section of the ATOMiK demo. There are 5 acts, each demonstrating a different capability: basic operations, self-inverse, parallel scaling, domain applications, and distributed merge.

**3-Node Network**
The demo simulates three separate computers (nodes) processing changes independently and then merging their results. This demonstrates ATOMiK's distributed computing capability — the ability to work across multiple machines without a central coordinator.

**Presentation Mode**
A demo option that adds explanatory narration text between acts, making it easier to present the demo to an audience who may not understand the technical details.

---

*40 terms defined. For terms not listed here, see the [ATOMiK for Non-Engineers](../01_Plain_Language/ATOMiK_for_Idiots.md) document for additional context.*

---

*ATOMiK — Delta-State Computing in Silicon*
*Patent Pending*
