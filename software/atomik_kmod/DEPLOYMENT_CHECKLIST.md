# ATOMiK Kernel Module — Deployment Checklist

Every item must PASS before shipping to customers.

---

## 1. BUILD & INSTALL

- [ ] **B-01** `make` compiles with zero errors on target kernel (warnings OK for -Wmissing-prototypes)
- [ ] **B-02** `make` compiles on Ubuntu 22.04 LTS (kernel 5.15)
- [ ] **B-03** `make` compiles on Ubuntu 24.04 LTS (kernel 6.8)
- [ ] **B-04** `make` compiles on latest mainline kernel (6.17+)
- [ ] **B-05** `tools/Makefile` builds atomik-status with zero errors/warnings
- [ ] **B-06** `install.sh` runs end-to-end without errors on fresh system
- [ ] **B-07** `install.sh` upgrades cleanly over existing install (removes old DKMS first)
- [ ] **B-08** `uninstall.sh` removes all installed files, DKMS, systemd, udev, group
- [ ] **B-09** `uninstall.sh --purge` also removes /var/lib/atomik/
- [ ] **B-10** DKMS rebuilds automatically on kernel upgrade (`apt upgrade` with new kernel)
- [ ] **B-11** `systemctl enable atomik` starts module on boot
- [ ] **B-12** `systemctl stop atomik` cleanly unloads module
- [ ] **B-13** Module loads and unloads 100 times in a loop without leaks (`kmemleak`)

## 2. FUNCTIONAL CORRECTNESS

- [ ] **F-01** CREATE_TABLE: allocates table, returns valid ID
- [ ] **F-02** CREATE_TABLE: rejects num_contexts=0 with -EINVAL
- [ ] **F-03** CREATE_TABLE: rejects num_contexts > 65536 with -EINVAL
- [ ] **F-04** CREATE_TABLE: fails gracefully at ATOMIK_MAX_TABLES (256)
- [ ] **F-05** DESTROY_TABLE: frees table, subsequent ops return -ENOENT
- [ ] **F-06** DESTROY_TABLE: rejects invalid table_id with -ENOENT
- [ ] **F-07** LOAD: sets initial state correctly
- [ ] **F-08** LOAD: rejects addr >= num_contexts with -EINVAL
- [ ] **F-09** ACCUM: XOR accumulates correctly (state = ref ^ acc)
- [ ] **F-10** ACCUM: rejects addr >= num_contexts with -EINVAL
- [ ] **F-11** READ: returns ref ^ acc correctly
- [ ] **F-12** READ: rejects addr >= num_contexts with -EINVAL
- [ ] **F-13** SWAP: returns old state, resets accumulator
- [ ] **F-14** SWAP: rejects addr >= num_contexts with -EINVAL
- [ ] **F-15** BATCH: executes mixed LOAD/ACCUM/READ/SWAP in sequence
- [ ] **F-16** BATCH: rejects num_ops=0 with -EINVAL
- [ ] **F-17** BATCH: rejects num_ops > 4096 with -EINVAL
- [ ] **F-18** BATCH: rejects invalid opcode with -EINVAL
- [ ] **F-19** BATCH: rejects addr >= num_contexts with -EINVAL
- [ ] **F-20** FP_REGISTER: pins pages, computes initial fingerprint
- [ ] **F-21** FP_CHECK: detects unchanged memory (changed=0)
- [ ] **F-22** FP_CHECK: detects changed memory (changed=1, correct pages_changed)
- [ ] **F-23** FP_UNREGISTER: unpins pages, subsequent check returns -ENOENT
- [ ] **F-24** GET_INFO: returns correct version, backend, stats
- [ ] **F-25** Multiple tables: independent state per table
- [ ] **F-26** Multiple fds: independent state per fd (open /dev/atomik twice)
- [ ] **F-27** fd close: all tables and FP regions cleaned up automatically
- [ ] **F-28** Delta commutativity: ACCUM(A) then ACCUM(B) == ACCUM(B) then ACCUM(A)
- [ ] **F-29** Self-inverse: ACCUM(X) then ACCUM(X) returns to original state
- [ ] **F-30** Identity: ACCUM(0) is a no-op
- [ ] **F-31** 64-bit full range: LOAD(0xFFFFFFFFFFFFFFFF) works correctly
- [ ] **F-32** Stats: ops_total increments correctly across all operations

## 3. SECURITY

- [ ] **S-01** Unprivileged user cannot open /dev/atomik (0660 root:atomik)
- [ ] **S-02** User in atomik group CAN open /dev/atomik
- [ ] **S-03** No kernel memory leak to userspace (all structs zeroed or fully populated before copy_to_user)
- [ ] **S-04** Invalid ioctl command returns -ENOTTY (not a crash)
- [ ] **S-05** NULL pointer in ioctl arg returns -EFAULT (not a crash)
- [ ] **S-06** Unmapped user pointer in ioctl arg returns -EFAULT
- [ ] **S-07** copy_to_user failure rolls back (table/region removed, not orphaned)
- [ ] **S-08** Concurrent CREATE + DESTROY from two threads: no UAF, no double-free
- [ ] **S-09** Concurrent ACCUM + DESTROY from two threads: no UAF
- [ ] **S-10** Concurrent FP_CHECK + FP_UNREGISTER from two threads: no UAF
- [ ] **S-11** FP_REGISTER with user_addr near ULONG_MAX: returns -EOVERFLOW (not wrap)
- [ ] **S-12** FP_REGISTER with length=0: returns -EINVAL
- [ ] **S-13** FP_REGISTER with length > 1GB: returns -EINVAL
- [ ] **S-14** Batch with crafted ops_ptr (unmapped memory): returns -EFAULT
- [ ] **S-15** Module survives `ioctl(fd, 0xFFFFFFFF, 0)` (garbage command)
- [ ] **S-16** Module survives rapid open/close cycles (1000 iterations)
- [ ] **S-17** Module survives fork + exec with inherited fd
- [ ] **S-18** `/sys/module/atomik/parameters/license_key` is root-only readable (0400)
- [ ] **S-19** `/etc/modprobe.d/atomik.conf` is root-only readable (0600) when license key present
- [ ] **S-20** No kernel oops/panic under any input combination (fuzz testing)

## 4. LICENSE ENFORCEMENT

- [ ] **L-01** No key: 90-day trial starts, trial_start file created
- [ ] **L-02** Trial persists across module unload/reload (same timestamp)
- [ ] **L-03** Trial persists across system reboot
- [ ] **L-04** Trial expires after 90 days: operations degrade gracefully (no errors)
- [ ] **L-05** Expired trial: GET_INFO still works (user can see "expired" status)
- [ ] **L-05b** Expired trial: LOAD/ACCUM silently dropped, READ/SWAP return 0
- [ ] **L-06** Expired trial: atomik-status shows "expired"
- [ ] **L-06b** Expired trial: one-time kernel log notification with purchase URL
- [ ] **L-06c** Expired trial: no application-visible errors (zero EPERM)
- [ ] **L-07** Valid professional key: accepted, shows "active"
- [ ] **L-08** Valid enterprise key: accepted, shows "active"
- [ ] **L-09** Forged key (random bytes): rejected, module refuses to load
- [ ] **L-10** 1-bit flip in valid key: rejected (HMAC fails)
- [ ] **L-11** Old format key (ATOMIK-XXXX-XXXX-XXXX-XXXX): rejected
- [ ] **L-12** Empty string key: enters trial mode (not rejected)
- [ ] **L-13** Wrong version byte: rejected with log warning
- [ ] **L-14** Wrong tier byte: rejected with log warning
- [ ] **L-15** Backdating system clock does NOT extend trial (trial_start is absolute)
- [ ] **L-16** Deleting trial_start file starts NEW trial (acceptable — requires root)
- [ ] **L-17** License key not visible via `strings atomik.ko | grep ATOMIK-`
- [ ] **L-18** HMAC secret not trivially extractable (obfuscation review)
- [ ] **L-19** Keygen tool generates unique keys per tier
- [ ] **L-20** Keygen verify rejects all invalid keys keygen generates no false positives

## 5. ANTI-PIRACY & TAMPER RESISTANCE

- [ ] **P-01** Module is GPL licensed (required for kernel symbols) but ioctl interface is proprietary
- [ ] **P-02** Cannot bypass license by calling ioctls directly (enforcement is in ioctl dispatch)
- [ ] **P-03** Cannot bypass trial by setting system clock forward then back
- [ ] **P-04** Trial state file permissions prevent non-root modification (0600)
- [ ] **P-05** /var/lib/atomik/ directory permissions prevent non-root access (0700)
- [ ] **P-06** HMAC secret is not in any user-facing file (only in .c source and compiled .ko)
- [ ] **P-07** Keygen tool is NOT included in customer distribution
- [ ] **P-08** Customer distribution does NOT include source code for atomik_license.c
- [ ] **P-09** Distribution package is a precompiled .ko + DKMS source (minus keygen/secret)
- [ ] **P-10** No debug printk exposing HMAC secret or key internals
- [ ] **P-11** License check uses constant-time comparison (crypto_memneq)
- [ ] **P-12** Module refuses to load with invalid key (not just warn)

## 6. STABILITY & RESOURCE MANAGEMENT

- [ ] **R-01** Module loads/unloads cleanly 100 times (no kmemleak reports)
- [ ] **R-02** No memory growth under sustained operation (1M ops, check /proc/meminfo)
- [ ] **R-03** ATOMIK_MAX_TABLES (256) enforced per fd — cannot exceed
- [ ] **R-04** ATOMIK_MAX_FP_REGIONS (1024) enforced per fd — cannot exceed
- [ ] **R-05** ATOMIK_MAX_BATCH_OPS (4096) enforced — cannot submit larger batch
- [ ] **R-06** Kernel memory freed on fd close (even without explicit DESTROY/UNREGISTER)
- [ ] **R-07** Pinned pages released on fd close / FP_UNREGISTER
- [ ] **R-08** sysfs attributes readable during high-load operation
- [ ] **R-09** Module loads on systems with 1 CPU (per-CPU stats still work)
- [ ] **R-10** Module loads on systems with 256+ CPUs (per-CPU stats aggregate correctly)

## 7. USERSPACE TOOLS

- [ ] **T-01** atomik-status: shows correct version
- [ ] **T-02** atomik-status: shows correct backend
- [ ] **T-03** atomik-status: shows correct license status for trial/active/expired
- [ ] **T-04** atomik-status: shows correct operation counts
- [ ] **T-05** atomik-status: shows correct fingerprint hit rate
- [ ] **T-06** atomik-status --quiet: prints one-line summary, exits 0
- [ ] **T-07** atomik-status: exits 1 with message when module not loaded
- [ ] **T-08** atomik-test: all 18 tests pass with valid license
- [ ] **T-09** atomik-test: graceful no-op behavior with expired license (no crashes, no errors)

## 8. PRIVACY

- [ ] **V-01** Fingerprinting does NOT expose page contents to userspace (only changed/unchanged flag)
- [ ] **V-02** No user memory contents copied to kernel log (printk)
- [ ] **V-03** No user memory contents exposed via sysfs
- [ ] **V-04** Stats are aggregate only (no per-user, per-process, or per-table breakdown in sysfs)
- [ ] **V-05** Module does not phone home or make network connections
- [ ] **V-06** No telemetry, analytics, or usage tracking beyond local sysfs counters
- [ ] **V-07** Trial state file contains only a Unix timestamp (no PII)

## 9. DOCUMENTATION & PACKAGING

- [ ] **D-01** Install instructions are clear and tested on fresh VM
- [ ] **D-02** Uninstall instructions work and leave no artifacts
- [ ] **D-03** License key entry documented (modprobe.d or insmod parameter)
- [ ] **D-04** Supported kernel versions documented (minimum 5.15+)
- [ ] **D-05** System requirements documented (x86_64 only for v0.1)
- [ ] **D-06** Known limitations documented (software backend only, XOR fingerprint caveats)
- [ ] **D-07** DKMS source package does NOT contain keygen tool or HMAC secret
- [ ] **D-08** Customer-facing source (if provided) has HMAC secret redacted
- [ ] **D-09** Release tarball tested on fresh Ubuntu 22.04 VM
- [ ] **D-10** Release tarball tested on fresh Ubuntu 24.04 VM

## 10. PRE-RELEASE FINAL CHECKS

- [ ] **X-01** `sudo dmesg` shows no warnings/errors from atomik module
- [ ] **X-02** No kernel taint flags set after module load
- [ ] **X-03** `lsmod | grep atomik` shows clean dependency list
- [ ] **X-04** `/dev/atomik` appears with correct permissions within 1 second of module load
- [ ] **X-05** System stable after 24-hour soak test with module loaded
- [ ] **X-06** Before/after performance metrics documented (CPU temp, fan, memory pressure)
- [ ] **X-07** Stripe checkout flow works end-to-end (purchase → receive key → install → verify)
- [ ] **X-08** Website download link works and serves correct package
- [ ] **X-09** All git changes committed and tagged with version number
- [ ] **X-10** Backup of HMAC signing key stored securely (not in git)

---

## Passing Criteria

- **ALL** items in sections 1-8 must pass
- Section 9 (Documentation) must have at least D-01 through D-06
- Section 10 (Pre-release) must have at least X-01 through X-06
- Any FAIL in sections 3 (Security) or 4 (License) is a **ship-blocker**

## Running the Automated Tests

```bash
# Smoke test (18 tests)
sudo insmod atomik.ko
sg atomik -c "./tools/atomik-test"
sudo rmmod atomik

# License tests
./tools/atomik-keygen.py --tier professional | xargs -I{} sudo insmod atomik.ko license_key={}
sg atomik -c "./tools/atomik-test"
sudo rmmod atomik

# Stress test
for i in $(seq 1 100); do
    sudo insmod atomik.ko
    sg atomik -c "./tools/atomik-test" > /dev/null
    sudo rmmod atomik
done
echo "100 load/unload cycles: OK"
```
