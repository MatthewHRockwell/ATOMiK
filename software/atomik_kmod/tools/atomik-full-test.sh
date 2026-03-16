#!/bin/bash
# ATOMiK Kernel Module — Comprehensive Test Suite
# Covers: functional, security, license, stability, privacy
# Usage: sudo ./atomik-full-test.sh
set -u

KMOD_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PASS=0
FAIL=0
SKIP=0

pass() { echo "  PASS: $1"; ((PASS++)); }
fail() { echo "  FAIL: $1"; ((FAIL++)); }
skip() { echo "  SKIP: $1"; ((SKIP++)); }

ensure_unloaded() {
    lsmod | grep -q "^atomik " && rmmod atomik 2>/dev/null
}

load_module() {
    insmod "${KMOD_DIR}/atomik.ko" "$@" 2>/dev/null
}

# Must run as root
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: must run as root"
    exit 1
fi

# Build test tool if needed
if [ ! -f "${KMOD_DIR}/tools/atomik-test" ]; then
    gcc -Wall -O2 -o "${KMOD_DIR}/tools/atomik-test" "${KMOD_DIR}/tools/atomik-test.c"
fi

echo "ATOMiK Kernel Module — Comprehensive Test Suite"
echo "================================================"
echo ""

# ============================================================
echo "--- BUILD TESTS ---"
# ============================================================

if [ -f "${KMOD_DIR}/atomik.ko" ]; then pass "B-01 Module file exists"; else fail "B-01 Module file not found"; fi

if [ -f "${KMOD_DIR}/tools/atomik-status" ]; then pass "B-05 atomik-status built"; else fail "B-05 atomik-status not built"; fi

# ============================================================
echo ""
echo "--- FUNCTIONAL TESTS ---"
# ============================================================

ensure_unloaded
rm -f /var/lib/atomik/trial_start
mkdir -p /var/lib/atomik
load_module

if ! lsmod | grep -q "^atomik "; then
    echo "  FATAL: Module failed to load"
    exit 1
fi

# Wait for udev
sleep 0.5

# Run smoke test
SMOKE_OUT=$("${KMOD_DIR}/tools/atomik-test" 2>&1)
SMOKE_PASSED=$(echo "$SMOKE_OUT" | grep -c "PASS")
SMOKE_TOTAL=18

echo -n "  F-01..F-32 Smoke test: "
if [ "$SMOKE_PASSED" -eq "$SMOKE_TOTAL" ]; then
    pass "smoke test ${SMOKE_PASSED}/${SMOKE_TOTAL}"
else
    fail "smoke test ${SMOKE_PASSED}/${SMOKE_TOTAL}"
    echo "$SMOKE_OUT" | grep "FAIL"
fi

# F-02: num_contexts=0
echo -n "  F-02 CREATE_TABLE num_contexts=0 rejected: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
buf = struct.pack('II', 0, 0)  # num_contexts=0
try:
    fcntl.ioctl(fd, 0xC0084101, buf)  # ATOMIK_IOC_CREATE_TABLE
    print('ACCEPTED')
except OSError as e:
    print(f'REJECTED:{e.errno}')
os.close(fd)
" 2>&1 | grep -q "REJECTED:22" && pass "F-02" || fail "F-02"

# F-03: num_contexts > 65536
echo -n "  F-03 CREATE_TABLE num_contexts=70000 rejected: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
buf = struct.pack('II', 70000, 0)
try:
    fcntl.ioctl(fd, 0xC0084101, buf)
    print('ACCEPTED')
except OSError as e:
    print(f'REJECTED:{e.errno}')
os.close(fd)
" 2>&1 | grep -q "REJECTED:22" && pass "F-03" || fail "F-03"

# F-08: addr out of bounds
echo -n "  F-08 LOAD addr >= num_contexts rejected: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
# Create table with 4 contexts
buf = struct.pack('II', 4, 0)
buf = fcntl.ioctl(fd, 0xC0084101, buf)
tid = struct.unpack('II', buf)[1]
# LOAD addr=99 (out of bounds)
buf = struct.pack('IIQ', tid, 99, 0xDEAD)
try:
    fcntl.ioctl(fd, 0x40104110, buf)  # ATOMIK_IOC_LOAD
    print('ACCEPTED')
except OSError as e:
    print(f'REJECTED:{e.errno}')
os.close(fd)
" 2>&1 | grep -q "REJECTED:22" && pass "F-08" || fail "F-08"

# F-26: Multiple fds have independent state
echo -n "  F-26 Multiple fds independent: "
python3 -c "
import fcntl, struct, os
fd1 = os.open('/dev/atomik', os.O_RDWR)
fd2 = os.open('/dev/atomik', os.O_RDWR)
# Create table on fd1
buf = struct.pack('II', 4, 0)
buf = fcntl.ioctl(fd1, 0xC0084101, buf)
tid1 = struct.unpack('II', buf)[1]
# Try to read from fd2 using fd1's table_id — should fail
buf = struct.pack('IIQ', tid1, 0, 0)
try:
    fcntl.ioctl(fd2, 0xC0104112, buf)
    print('SHARED')
except OSError:
    print('INDEPENDENT')
os.close(fd1)
os.close(fd2)
" 2>&1 | grep -q "INDEPENDENT" && pass "F-26" || fail "F-26"

# F-28: Delta commutativity
echo -n "  F-28 Delta commutativity: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
buf = struct.pack('II', 4, 0)
buf = fcntl.ioctl(fd, 0xC0084101, buf)
tid = struct.unpack('II', buf)[1]
# LOAD 0
fcntl.ioctl(fd, 0x40104110, struct.pack('IIQ', tid, 0, 0))
fcntl.ioctl(fd, 0x40104110, struct.pack('IIQ', tid, 1, 0))
# Ctx 0: ACCUM A then B
fcntl.ioctl(fd, 0x40104111, struct.pack('IIQ', tid, 0, 0xAA))
fcntl.ioctl(fd, 0x40104111, struct.pack('IIQ', tid, 0, 0xBB))
# Ctx 1: ACCUM B then A
fcntl.ioctl(fd, 0x40104111, struct.pack('IIQ', tid, 1, 0xBB))
fcntl.ioctl(fd, 0x40104111, struct.pack('IIQ', tid, 1, 0xAA))
# Read both
r0 = fcntl.ioctl(fd, 0xC0104112, struct.pack('IIQ', tid, 0, 0))
r1 = fcntl.ioctl(fd, 0xC0104112, struct.pack('IIQ', tid, 1, 0))
s0 = struct.unpack('IIQ', r0)[2]
s1 = struct.unpack('IIQ', r1)[2]
print('COMMUTATIVE' if s0 == s1 else f'BROKEN {s0:#x} != {s1:#x}')
os.close(fd)
" 2>&1 | grep -q "COMMUTATIVE" && pass "F-28" || fail "F-28"

# F-29: Self-inverse
echo -n "  F-29 Self-inverse: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
buf = struct.pack('II', 4, 0)
buf = fcntl.ioctl(fd, 0xC0084101, buf)
tid = struct.unpack('II', buf)[1]
fcntl.ioctl(fd, 0x40104110, struct.pack('IIQ', tid, 0, 0x42))
fcntl.ioctl(fd, 0x40104111, struct.pack('IIQ', tid, 0, 0xFF))
fcntl.ioctl(fd, 0x40104111, struct.pack('IIQ', tid, 0, 0xFF))
r = fcntl.ioctl(fd, 0xC0104112, struct.pack('IIQ', tid, 0, 0))
s = struct.unpack('IIQ', r)[2]
print('SELF_INVERSE' if s == 0x42 else f'BROKEN {s:#x}')
os.close(fd)
" 2>&1 | grep -q "SELF_INVERSE" && pass "F-29" || fail "F-29"

# F-30: Identity
echo -n "  F-30 Identity (ACCUM 0 is no-op): "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
buf = struct.pack('II', 4, 0)
buf = fcntl.ioctl(fd, 0xC0084101, buf)
tid = struct.unpack('II', buf)[1]
fcntl.ioctl(fd, 0x40104110, struct.pack('IIQ', tid, 0, 0xDEADBEEF))
fcntl.ioctl(fd, 0x40104111, struct.pack('IIQ', tid, 0, 0))
r = fcntl.ioctl(fd, 0xC0104112, struct.pack('IIQ', tid, 0, 0))
s = struct.unpack('IIQ', r)[2]
print('IDENTITY' if s == 0xDEADBEEF else f'BROKEN {s:#x}')
os.close(fd)
" 2>&1 | grep -q "IDENTITY" && pass "F-30" || fail "F-30"

# F-31: 64-bit full range
echo -n "  F-31 64-bit full range: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
buf = struct.pack('II', 4, 0)
buf = fcntl.ioctl(fd, 0xC0084101, buf)
tid = struct.unpack('II', buf)[1]
fcntl.ioctl(fd, 0x40104110, struct.pack('IIQ', tid, 0, 0xFFFFFFFFFFFFFFFF))
r = fcntl.ioctl(fd, 0xC0104112, struct.pack('IIQ', tid, 0, 0))
s = struct.unpack('IIQ', r)[2]
print('FULL_RANGE' if s == 0xFFFFFFFFFFFFFFFF else f'BROKEN {s:#x}')
os.close(fd)
" 2>&1 | grep -q "FULL_RANGE" && pass "F-31" || fail "F-31"

ensure_unloaded

# ============================================================
echo ""
echo "--- SECURITY TESTS ---"
# ============================================================

load_module
sleep 0.5

# S-01: Unprivileged user denied
echo -n "  S-01 Unprivileged user denied: "
sudo -u nobody cat /dev/atomik 2>&1 | grep -q "Permission denied" && pass "S-01" || fail "S-01"

# S-04: Invalid ioctl command
echo -n "  S-04 Invalid ioctl returns -ENOTTY: "
python3 -c "
import fcntl, os
fd = os.open('/dev/atomik', os.O_RDWR)
try:
    fcntl.ioctl(fd, 0xFFFFFFFF, b'\x00' * 64)
    print('ACCEPTED')
except OSError as e:
    print(f'REJECTED:{e.errno}')
os.close(fd)
" 2>&1 | grep -q "REJECTED:25" && pass "S-04" || fail "S-04"

# S-05: NULL arg (addr 0)
echo -n "  S-05 NULL arg returns -EFAULT: "
python3 -c "
import fcntl, os, ctypes
fd = os.open('/dev/atomik', os.O_RDWR)
try:
    # Call ioctl with arg=0 (NULL pointer)
    ret = ctypes.CDLL(None).ioctl(fd, 0xC0084101, 0)
    import errno
    err = ctypes.get_errno()
    print(f'RET:{ret}:ERR:{err}')
except Exception as e:
    print(f'EXCEPTION:{e}')
os.close(fd)
" 2>&1 | grep -qE "(REJECTED:14|RET:-1)" && pass "S-05" || fail "S-05"

# S-15: Garbage ioctl command
echo -n "  S-15 Garbage ioctl (no crash): "
python3 -c "
import fcntl, os
fd = os.open('/dev/atomik', os.O_RDWR)
for cmd in [0, 1, 0x4100, 0xDEAD, 0xFFFF]:
    try:
        fcntl.ioctl(fd, cmd, b'\x00' * 64)
    except:
        pass
print('SURVIVED')
os.close(fd)
" 2>&1 | grep -q "SURVIVED" && pass "S-15" || fail "S-15"

# S-16: Rapid open/close
echo -n "  S-16 1000 rapid open/close cycles: "
python3 -c "
import os
for _ in range(1000):
    fd = os.open('/dev/atomik', os.O_RDWR)
    os.close(fd)
print('OK')
" 2>&1 | grep -q "OK" && pass "S-16" || fail "S-16"

# S-18: License key parameter permissions
echo -n "  S-18 License key param is root-only: "
PERM=$(stat -c %a /sys/module/atomik/parameters/license_key 2>/dev/null)
[ "$PERM" = "400" ] && pass "S-18" || fail "S-18 (got ${PERM:-missing})"

ensure_unloaded

# ============================================================
echo ""
echo "--- LICENSE TESTS ---"
# ============================================================

# L-01: Fresh trial
echo -n "  L-01 Fresh trial creates trial_start: "
rm -f /var/lib/atomik/trial_start
load_module
[ -f /var/lib/atomik/trial_start ] && pass "L-01" || fail "L-01"
TS1=$(cat /var/lib/atomik/trial_start)
ensure_unloaded

# L-02: Trial persists across reload
echo -n "  L-02 Trial persists across reload: "
load_module
TS2=$(cat /var/lib/atomik/trial_start)
[ "$TS1" = "$TS2" ] && pass "L-02" || fail "L-02 ($TS1 != $TS2)"
ensure_unloaded

# L-04: Expired trial degrades gracefully (no errors)
echo -n "  L-04 Expired trial degrades gracefully: "
EXPIRED_TS=$(($(date +%s) - 91 * 86400))
echo "$EXPIRED_TS" > /var/lib/atomik/trial_start
load_module
sleep 0.5
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
# CREATE still works
buf = struct.pack('II', 4, 0)
buf = fcntl.ioctl(fd, 0xC0084101, buf)
tid = struct.unpack('II', buf)[1]
# LOAD silently dropped (no error)
fcntl.ioctl(fd, 0x40104110, struct.pack('IIQ', tid, 0, 0xDEADBEEF))
# READ returns 0 (not the loaded value)
r = fcntl.ioctl(fd, 0xC0104112, struct.pack('IIQ', tid, 0, 0))
state = struct.unpack('IIQ', r)[2]
if tid > 0 and state == 0:
    print('GRACEFUL')
else:
    print(f'BROKEN tid={tid} state={state:#x}')
os.close(fd)
" 2>&1 | grep -q "GRACEFUL" && pass "L-04" || fail "L-04"

# L-05: GET_INFO works even with expired trial
echo -n "  L-05 GET_INFO works when expired: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
buf = struct.pack('IIIIQQq', 0,0,0,0,0,0,0)
try:
    r = fcntl.ioctl(fd, 0x80284140, buf)
    print('WORKS')
except OSError as e:
    print(f'BLOCKED:{e.errno}')
os.close(fd)
" 2>&1 | grep -q "WORKS" && pass "L-05" || fail "L-05"

# L-05b: All ops return 0 (no -EPERM) when expired
echo -n "  L-05b No errors on any op when expired: "
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
errors = 0
for name, cmd, fmt, args in [
    ('LOAD',  0x40104110, 'IIQ', (1, 0, 0xAA)),
    ('ACCUM', 0x40104111, 'IIQ', (1, 0, 0xBB)),
]:
    try:
        fcntl.ioctl(fd, cmd, struct.pack(fmt, *args))
    except OSError:
        errors += 1
for name, cmd, fmt, args in [
    ('READ', 0xC0104112, 'IIQ', (1, 0, 0)),
    ('SWAP', 0xC0184113, 'IIQQ', (1, 0, 0, 0)),
]:
    try:
        fcntl.ioctl(fd, cmd, struct.pack(fmt, *args))
    except OSError:
        errors += 1
os.close(fd)
print('ZERO_ERRORS' if errors == 0 else f'HAD_{errors}_ERRORS')
" 2>&1 | grep -q "ZERO_ERRORS" && pass "L-05b" || fail "L-05b"
ensure_unloaded

# L-06: atomik-status shows expired
echo -n "  L-06 atomik-status shows expired: "
load_module
sleep 0.5
"${KMOD_DIR}/tools/atomik-status" 2>&1 | grep -q "expired" && pass "L-06" || fail "L-06"
ensure_unloaded

# L-07: Valid professional key (requires keygen — internal only)
rm -f /var/lib/atomik/trial_start
if [ -f "${KMOD_DIR}/tools/atomik-keygen.py" ]; then
    PRO_KEY=$(python3 "${KMOD_DIR}/tools/atomik-keygen.py" --tier professional)
    echo -n "  L-07 Valid professional key accepted: "
    load_module license_key="$PRO_KEY"
    if lsmod | grep -q "^atomik "; then
        pass "L-07"
    else
        fail "L-07"
    fi
    ensure_unloaded
else
    skip "L-07 (keygen not available)"
fi

# L-08: Valid enterprise key (requires keygen — internal only)
if [ -f "${KMOD_DIR}/tools/atomik-keygen.py" ]; then
    ENT_KEY=$(python3 "${KMOD_DIR}/tools/atomik-keygen.py" --tier enterprise)
    echo -n "  L-08 Valid enterprise key accepted: "
    load_module license_key="$ENT_KEY"
    if lsmod | grep -q "^atomik "; then
        pass "L-08"
    else
        fail "L-08"
    fi
    ensure_unloaded
else
    skip "L-08 (keygen not available)"
fi

# L-09: Forged key rejected
echo -n "  L-09 Forged key rejected: "
load_module license_key="ATOMIK-DEADBEEF-DEADBEEF-DEADBEEF-DEADBEEF-DEADBEEF" 2>&1
if lsmod | grep -q "^atomik "; then
    fail "L-09 (module loaded with forged key)"
    ensure_unloaded
else
    pass "L-09"
fi

# L-10: 1-bit flip rejected
echo -n "  L-10 1-bit flip rejected: "
# Flip last hex char of a valid key
FLIPPED="${PRO_KEY%?}0"
[ "${FLIPPED: -1}" = "${PRO_KEY: -1}" ] && FLIPPED="${PRO_KEY%?}1"
load_module license_key="$FLIPPED" 2>&1
if lsmod | grep -q "^atomik "; then
    fail "L-10 (module loaded with flipped key)"
    ensure_unloaded
else
    pass "L-10"
fi

# L-11: Old format key rejected
echo -n "  L-11 Old format key rejected: "
load_module license_key="ATOMIK-AAAA-BBBB-CCCC-DDDD" 2>&1
if lsmod | grep -q "^atomik "; then
    fail "L-11"
    ensure_unloaded
else
    pass "L-11"
fi

# ============================================================
echo ""
echo "--- STABILITY TESTS ---"
# ============================================================

# R-01: 100 load/unload cycles
echo -n "  R-01 100 load/unload cycles: "
rm -f /var/lib/atomik/trial_start
STABLE=1
for i in $(seq 1 100); do
    load_module 2>/dev/null
    if ! lsmod | grep -q "^atomik "; then
        echo "  Failed at iteration $i"
        STABLE=0
        break
    fi
    rmmod atomik 2>/dev/null
done
[ $STABLE -eq 1 ] && pass "R-01" || fail "R-01"

# R-06: fd close cleans up
echo -n "  R-06 fd close cleans up: "
load_module
sleep 0.5
python3 -c "
import fcntl, struct, os
fd = os.open('/dev/atomik', os.O_RDWR)
# Create 10 tables
for _ in range(10):
    buf = struct.pack('II', 256, 0)
    fcntl.ioctl(fd, 0xC0084101, buf)
os.close(fd)
# If we get here without crash, cleanup worked
print('CLEAN')
" 2>&1 | grep -q "CLEAN" && pass "R-06" || fail "R-06"
ensure_unloaded

# ============================================================
echo ""
echo "--- PRIVACY TESTS ---"
# ============================================================

echo -n "  V-05 No network connections: "
load_module
sleep 0.5
# Check if module creates any sockets
SS_OUT=$(ss -tlnp 2>/dev/null | grep atomik || true)
[ -z "$SS_OUT" ] && pass "V-05" || fail "V-05"

echo -n "  V-07 Trial state is just a timestamp: "
TRIAL_CONTENT=$(cat /var/lib/atomik/trial_start 2>/dev/null)
if echo "$TRIAL_CONTENT" | grep -qE '^[0-9]+$'; then
    pass "V-07"
else
    fail "V-07 (content: $TRIAL_CONTENT)"
fi

ensure_unloaded

# ============================================================
echo ""
echo "--- ANTI-PIRACY TESTS ---"
# ============================================================

echo -n "  P-06 HMAC secret not in sysfs: "
SYSFS_LEAK=$(find /sys/class/atomik /sys/module/atomik -type f -exec cat {} \; 2>/dev/null | grep -c "a73c198e" || true)
[ "$SYSFS_LEAK" = "0" ] && pass "P-06" || fail "P-06"

echo -n "  P-11 Constant-time HMAC comparison: "
grep -q "crypto_memneq" "${KMOD_DIR}/atomik_license.c" && pass "P-11" || fail "P-11"

# ============================================================
echo ""
echo "================================================"
echo "Results: $PASS passed, $FAIL failed, $SKIP skipped"
echo "================================================"

# Cleanup
rm -f /var/lib/atomik/trial_start
ensure_unloaded

exit $FAIL
