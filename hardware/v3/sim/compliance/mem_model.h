// =============================================================================
// ATOMiK v3 Memory Model for Compliance Testing
//
// Flat memory with tohost/fromhost monitoring for riscv-tests.
// Provides 32-bit bus interface (valid/ready protocol).
// =============================================================================

#ifndef MEM_MODEL_H
#define MEM_MODEL_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <map>

class MemModel {
public:
    static const uint32_t MEM_BASE = 0x80000000;
    static const uint32_t MEM_SIZE = 0x01000000;  // 16 MB (0x80000000 - 0x80FFFFFF)

    MemModel() : tohost_addr_(0), tohost_value_(0), tohost_written_(false) {
        mem_ = new uint8_t[MEM_SIZE];
        memset(mem_, 0, MEM_SIZE);
    }

    ~MemModel() {
        delete[] mem_;
    }

    void set_tohost_addr(uint32_t addr) { tohost_addr_ = addr; }
    uint32_t get_tohost_addr() const { return tohost_addr_; }
    uint64_t get_tohost_value() const { return tohost_value_; }
    bool tohost_written() const { return tohost_written_; }
    void clear_tohost() { tohost_written_ = false; tohost_value_ = 0; }

    // Translate absolute address to array index; returns false if out of range
    bool translate(uint32_t addr, uint32_t &idx) const {
        if (addr >= MEM_BASE && addr < MEM_BASE + MEM_SIZE) {
            idx = addr - MEM_BASE;
            return true;
        }
        return false;
    }

    // Write a byte at absolute address
    void write_byte(uint32_t addr, uint8_t val) {
        uint32_t idx;
        if (translate(addr, idx))
            mem_[idx] = val;
    }

    // Read a byte at absolute address
    uint8_t read_byte(uint32_t addr) const {
        uint32_t idx;
        if (translate(addr, idx))
            return mem_[idx];
        return 0;
    }

    // Write a block of data
    void write_block(uint32_t addr, const uint8_t *data, uint32_t len) {
        for (uint32_t i = 0; i < len; i++) {
            write_byte(addr + i, data[i]);
        }
    }

    // 32-bit bus read (word-aligned)
    uint32_t bus_read(uint32_t addr) const {
        addr &= ~3u;  // Word-align
        uint32_t idx;
        if (translate(addr, idx) && idx + 3 < MEM_SIZE) {
            uint32_t val = 0;
            val |= (uint32_t)mem_[idx + 0];
            val |= (uint32_t)mem_[idx + 1] << 8;
            val |= (uint32_t)mem_[idx + 2] << 16;
            val |= (uint32_t)mem_[idx + 3] << 24;
            return val;
        }
        return 0;
    }

    // 32-bit bus write with byte strobes
    void bus_write(uint32_t addr, uint32_t data, uint8_t wstrb) {
        addr &= ~3u;  // Word-align

        // Check for tohost write (using absolute address)
        if (addr == tohost_addr_ && wstrb != 0) {
            if (wstrb & 0x1) { tohost_value_ = (tohost_value_ & ~0xFFULL) | (data & 0xFF); }
            if (wstrb & 0x2) { tohost_value_ = (tohost_value_ & ~0xFF00ULL) | (data & 0xFF00); }
            if (wstrb & 0x4) { tohost_value_ = (tohost_value_ & ~0xFF0000ULL) | (data & 0xFF0000); }
            if (wstrb & 0x8) { tohost_value_ = (tohost_value_ & ~0xFF000000ULL) | (data & 0xFF000000); }
            if (tohost_value_ != 0)
                tohost_written_ = true;
        }
        if (addr == (tohost_addr_ + 4) && wstrb != 0) {
            uint64_t upper = 0;
            if (wstrb & 0x1) upper |= (uint64_t)(data & 0xFF) << 32;
            if (wstrb & 0x2) upper |= (uint64_t)((data >> 8) & 0xFF) << 40;
            if (wstrb & 0x4) upper |= (uint64_t)((data >> 16) & 0xFF) << 48;
            if (wstrb & 0x8) upper |= (uint64_t)((data >> 24) & 0xFF) << 56;
            tohost_value_ |= upper;
            if (tohost_value_ != 0)
                tohost_written_ = true;
        }

        uint32_t idx;
        if (translate(addr, idx) && idx + 3 < MEM_SIZE) {
            if (wstrb & 0x1) mem_[idx + 0] = (data >> 0) & 0xFF;
            if (wstrb & 0x2) mem_[idx + 1] = (data >> 8) & 0xFF;
            if (wstrb & 0x4) mem_[idx + 2] = (data >> 16) & 0xFF;
            if (wstrb & 0x8) mem_[idx + 3] = (data >> 24) & 0xFF;
        }
    }

    // Handle bus transaction (single-cycle ready)
    // Returns true if transaction occurred
    bool tick(uint8_t valid, uint32_t addr, uint32_t wdata, uint8_t wstrb,
              uint32_t &rdata, uint8_t &ready) {
        ready = 0;
        rdata = 0;
        if (valid) {
            ready = 1;
            if (wstrb) {
                bus_write(addr, wdata, wstrb);
            } else {
                rdata = bus_read(addr);
            }
            return true;
        }
        return false;
    }

private:
    uint8_t *mem_;
    uint32_t tohost_addr_;
    uint64_t tohost_value_;
    bool tohost_written_;
};

#endif // MEM_MODEL_H
