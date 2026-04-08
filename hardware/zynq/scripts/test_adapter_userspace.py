#!/usr/bin/env python3
"""ATOMiK CFU Adapter — Linux Userspace Validation via devmem over serial."""
import serial, time, sys

PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB2'
ser = serial.Serial(PORT, 115200, timeout=5)
ser.reset_input_buffer()

def run(c, d=0.5):
    ser.write((c+'\n').encode()); time.sleep(d)
    return ser.read(8192).decode('utf-8', errors='replace')

def dr(a):
    o = run('devmem 0x%08X' % a)
    for l in o.split('\n'):
        l = l.strip()
        if l.startswith('0x'):
            try: return int(l, 16)
            except: pass
    return 0

def dw(a, v):
    run('devmem 0x%08X 32 0x%08X' % (a, v), 0.3)

B = 0xF0020000; p = 0; f = 0

def chk(n, g, e):
    global p, f
    if g == e: print('  PASS: %s' % n); p += 1
    else: print('  FAIL: %s got=0x%016X exp=0x%016X' % (n, g, e)); f += 1

def ld(a, v):
    dw(B+4, a); dw(B+8, v & 0xFFFFFFFF); dw(B, 0)
    dw(B+8, (v >> 32) & 0xFFFFFFFF); dw(B, 4)

def acc(d):
    dw(B+4, d & 0xFFFFFFFF); dw(B, 1)
    dw(B+4, (d >> 32) & 0xFFFFFFFF); dw(B, 5)

def rd():
    dw(B, 2); lo = dr(B + 0xC)
    dw(B, 6); hi = dr(B + 0xC)
    return (hi << 32) | lo

print('=' * 60)
print('ATOMiK CFU Adapter — Linux Userspace Validation')
print('Adapter: 0x%08X | Zynq XC7Z020 + VexRiscvSMP' % B)
print('=' * 60)
print('STATUS: 0x%08X' % dr(B + 0x10))

ld(0, 0xDEADBEEFCAFEBABE); chk('T1 LOAD/READ', rd(), 0xDEADBEEFCAFEBABE)
acc(0x1111111111111111); chk('T2 ACCUM', rd(), 0xDEADBEEFCAFEBABE ^ 0x1111111111111111)
acc(0x1111111111111111); chk('T3 Self-inverse', rd(), 0xDEADBEEFCAFEBABE)
acc(0); chk('T4 Identity', rd(), 0xDEADBEEFCAFEBABE)

ld(0, 0); acc(0xAA); acc(0x55); ab = rd()
ld(0, 0); acc(0x55); acc(0xAA); ba = rd()
chk('T5 Commutativity', ab, ba)

ld(0, 0x1111111111111111); ld(1, 0x2222222222222222)
chk('T6a Addr1', rd(), 0x2222222222222222)
ld(0, 0x1111111111111111); chk('T6b Addr0', rd(), 0x1111111111111111)

ld(5, 0xAAAA0000BBBB0000); acc(0x0000FFFF0000FFFF)
e = 0xAAAA0000BBBB0000 ^ 0x0000FFFF0000FFFF
dw(B+4, 5); dw(B, 3)  # SWAP
chk('T7 SWAP', rd(), e); chk('T8 SWAP-clr', rd(), e)

print('\n' + '=' * 60)
print('%d PASS, %d FAIL / %d' % (p, f, p + f))
if f == 0: print('ALL TESTS PASSED')
print('=' * 60)
ser.close()
sys.exit(1 if f else 0)
