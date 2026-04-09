/**
 * ATOMiK Core — Delta-state algebra for JavaScript/TypeScript.
 *
 * Same 4-operation algebra as the Python SDK:
 *   LOAD  — Set initial reference state
 *   ACCUM — XOR delta into accumulator
 *   READ  — Reconstruct current state (reference XOR accumulator)
 *   SWAP  — Atomic read-and-reset (snapshot + new epoch)
 *
 * All operations are O(1) in time and space.
 * Backed by 108 Lean4 theorems proving commutativity, associativity,
 * self-inverse, and identity.
 *
 * @example
 * ```ts
 * import { AtomikContext } from '@atomik/core';
 *
 * const ctx = new AtomikContext();
 * ctx.load(0xDEADBEEF);
 * ctx.accum(0x000000FF);
 * console.log(ctx.read().toString(16)); // deadbe10
 * ```
 *
 * @packageDocumentation
 */

/**
 * 32-bit XOR helper (JS bitwise ops are 32-bit).
 * For 64-bit, use BigInt variant.
 */
function xor32(a: number, b: number): number {
  return (a ^ b) >>> 0;
}

/**
 * Single ATOMiK delta-state context (32-bit).
 *
 * Implements the 4-operation algebra with O(1) time and space.
 * Uses 32-bit unsigned integers (JS bitwise limitation).
 * For 64-bit, use AtomikContext64 with BigInt.
 */
export class AtomikContext {
  private _reference: number = 0;
  private _accumulator: number = 0;
  private _deltaCount: number = 0;

  constructor(initialState: number = 0) {
    this._reference = initialState >>> 0;
  }

  /** LOAD: Set reference state, clear accumulator. */
  load(value: number): void {
    this._reference = value >>> 0;
    this._accumulator = 0;
    this._deltaCount = 0;
  }

  /** ACCUM: XOR delta into the accumulator. Order-independent. */
  accum(delta: number): void {
    this._accumulator = xor32(this._accumulator, delta);
    this._deltaCount++;
  }

  /** READ: Reconstruct current state = reference XOR accumulator. */
  read(): number {
    return xor32(this._reference, this._accumulator);
  }

  /** SWAP: Atomic snapshot + new epoch. Returns previous state. */
  swap(newReference?: number): number {
    const current = this.read();
    this._reference = newReference !== undefined ? (newReference >>> 0) : current;
    this._accumulator = 0;
    this._deltaCount = 0;
    return current;
  }

  /** Undo a previously applied delta (self-inverse: rollback = accum). */
  rollback(delta: number): void {
    this.accum(delta);
  }

  get reference(): number { return this._reference; }
  get accumulator(): number { return this._accumulator; }
  get deltaCount(): number { return this._deltaCount; }
  get isClean(): boolean { return this._accumulator === 0; }
}

/**
 * Multi-context state table (32-bit).
 *
 * Maps integer addresses to independent AtomikContext instances.
 */
export class AtomikTable {
  private _contexts: AtomikContext[];
  readonly numContexts: number;

  constructor(numContexts: number = 256) {
    this.numContexts = numContexts;
    this._contexts = Array.from({ length: numContexts }, () => new AtomikContext());
  }

  load(addr: number, value: number): void {
    this._checkAddr(addr);
    this._contexts[addr].load(value);
  }

  accum(addr: number, delta: number): void {
    this._checkAddr(addr);
    this._contexts[addr].accum(delta);
  }

  read(addr: number): number {
    this._checkAddr(addr);
    return this._contexts[addr].read();
  }

  swap(addr: number, newRef?: number): number {
    this._checkAddr(addr);
    return this._contexts[addr].swap(newRef);
  }

  private _checkAddr(addr: number): void {
    if (addr < 0 || addr >= this.numContexts) {
      throw new RangeError(`Address ${addr} out of range [0, ${this.numContexts})`);
    }
  }
}

/**
 * XOR-based change detection fingerprint (32-bit).
 */
export class Fingerprint {
  private _ctx = new AtomikContext();

  /** Load reference data. */
  load(data: Uint8Array): number {
    const fp = this._reduce(data);
    this._ctx.load(fp);
    return fp;
  }

  /** Update with new data and check for changes. */
  update(data: Uint8Array): number {
    const fp = this._reduce(data);
    // Set accumulator to ref XOR current so read() = current
    const delta = xor32(this._ctx.reference, fp);
    this._ctx.load(this._ctx.reference);
    this._ctx.accum(delta);
    return fp;
  }

  /** True if data changed since last load. */
  get changed(): boolean {
    return this._ctx.accumulator !== 0;
  }

  /** XOR delta between reference and current fingerprint. */
  get delta(): number {
    return this._ctx.accumulator;
  }

  /** Reset to clean state. */
  reset(): void {
    this._ctx.load(this._ctx.reference);
  }

  private _reduce(data: Uint8Array): number {
    let fp = 0;
    const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
    const fullWords = Math.floor(data.length / 4);
    for (let i = 0; i < fullWords; i++) {
      fp = xor32(fp, view.getUint32(i * 4, true));
    }
    // Handle remaining bytes
    const remaining = data.length % 4;
    if (remaining > 0) {
      let last = 0;
      for (let i = 0; i < remaining; i++) {
        last |= data[fullWords * 4 + i] << (i * 8);
      }
      fp = xor32(fp, last);
    }
    return fp;
  }
}

/**
 * Delta message for network transport.
 * 12 bytes: addr(u32) + delta(u32) + seq(u32).
 */
export class DeltaMessage {
  constructor(
    public readonly addr: number,
    public readonly delta: number,
    public readonly seq: number,
  ) {}

  /** Serialize to 12-byte ArrayBuffer (network byte order). */
  toBytes(): ArrayBuffer {
    const buf = new ArrayBuffer(12);
    const view = new DataView(buf);
    view.setUint32(0, this.addr, false);
    view.setUint32(4, this.delta, false);
    view.setUint32(8, this.seq, false);
    return buf;
  }

  /** Deserialize from 12-byte ArrayBuffer. */
  static fromBytes(buf: ArrayBuffer): DeltaMessage {
    if (buf.byteLength !== 12) throw new Error(`Expected 12 bytes, got ${buf.byteLength}`);
    const view = new DataView(buf);
    return new DeltaMessage(
      view.getUint32(0, false),
      view.getUint32(4, false),
      view.getUint32(8, false),
    );
  }

  toJSON(): { addr: number; delta: number; seq: number } {
    return { addr: this.addr, delta: this.delta, seq: this.seq };
  }
}

/**
 * Synchronized table with delta callback for network transport.
 */
export class SyncTable {
  private _table: AtomikTable;
  private _onDelta: ((msg: DeltaMessage) => void) | null = null;
  private _seq = 0;

  constructor(numContexts: number = 256) {
    this._table = new AtomikTable(numContexts);
  }

  /** Register callback for outbound deltas. */
  onDelta(callback: ((msg: DeltaMessage) => void) | null): void {
    this._onDelta = callback;
  }

  /** Write a value and broadcast the delta. */
  put(addr: number, value: number): void {
    const old = this._table.read(addr);
    const delta = xor32(old, value);
    if (delta === 0) return; // no-op
    this._table.accum(addr, delta);
    if (this._onDelta) {
      this._onDelta(new DeltaMessage(addr, delta, this._seq++));
    }
  }

  /** Read current state at address. */
  get(addr: number): number {
    return this._table.read(addr);
  }

  /** Apply a remote delta (no callback fired). */
  receive(msg: DeltaMessage): void {
    this._table.accum(msg.addr, msg.delta);
  }
}
