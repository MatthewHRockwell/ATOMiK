# ATOMiK Delta-State Accelerator — Linux Kernel Module

ATOMiK is a system-level delta-state acceleration engine. The kernel module
provides transparent page fingerprinting, lock-free accumulation, and atomic
state reconstruction via `/dev/atomik`.

## Quick Start

```bash
sudo ./install.sh                          # Install with 90-day free trial
sudo ./install.sh --license ATOMIK-XXXX-...  # Install with license key
```

After installation:

```bash
atomik-status    # Check module status and live metrics
atomik-test      # Run functional smoke tests
atomik-bench     # Benchmark ATOMiK vs naive approaches
```

## System Requirements

- Linux kernel >= 5.15
- x86_64 architecture
- GCC and kernel headers (`sudo apt install build-essential linux-headers-$(uname -r)`)
- DKMS (`sudo apt install dkms`)
- Root access for installation

## What Gets Installed

| Component | Location |
|-----------|----------|
| Kernel module (DKMS) | `/lib/modules/$(uname -r)/updates/atomik.ko` |
| Device node | `/dev/atomik` (auto-created by udev) |
| Status tool | `/usr/local/bin/atomik-status` |
| Test tool | `/usr/local/bin/atomik-test` |
| Benchmark tool | `/usr/local/bin/atomik-bench` |
| Full test suite | `/usr/local/bin/atomik-full-test` |
| Systemd service | `/etc/systemd/system/atomik.service` |
| Modprobe config | `/etc/modprobe.d/atomik.conf` |
| Udev rule | `/etc/udev/rules.d/99-atomik.rules` |
| Trial state | `/var/lib/atomik/` |

## Licensing

- **Trial**: 90-day free trial. All features enabled. No license key required.
- **Professional** ($99/mo): Full kernel module with page fingerprinting.
- **Enterprise** ($499/mo): Kernel module + hardware acceleration support.

Purchase a license at [atomik.tech](https://atomik.tech).

After purchase, apply your key:

```bash
sudo ./install.sh --license ATOMIK-XXXX-XXXX-XXXX-XXXX-XXXX
```

Or update an existing installation:

```bash
echo 'options atomik license_key=ATOMIK-XXXX-XXXX-XXXX-XXXX-XXXX' | sudo tee /etc/modprobe.d/atomik.conf
sudo chmod 600 /etc/modprobe.d/atomik.conf
sudo modprobe -r atomik && sudo modprobe atomik
```

## Uninstall

```bash
sudo atomik-uninstall          # Keep trial state
sudo atomik-uninstall --purge  # Remove everything
```

## Device Access

By default, `/dev/atomik` is accessible to `root` and members of the `atomik`
group. The installer adds the installing user to this group automatically.
Re-login for group membership to take effect.

To add another user:

```bash
sudo usermod -aG atomik USERNAME
```

## Monitoring

```bash
atomik-status                  # One-shot status display
watch -n1 atomik-status        # Live dashboard
cat /sys/class/atomik/atomik0/ops_total     # Raw sysfs counter
```

## Troubleshooting

**Module won't load**
```bash
dmesg | tail -20               # Check kernel log for errors
modinfo atomik                 # Verify module is installed
```

**Permission denied on /dev/atomik**
```bash
groups                         # Verify you're in the 'atomik' group
ls -la /dev/atomik             # Check device permissions
```

**DKMS build fails**
```bash
sudo apt install linux-headers-$(uname -r)  # Install matching headers
dkms status                    # Check DKMS state
```

**Trial expired**
```bash
atomik-status                  # Shows "expired" if trial has ended
```

The module continues to load after trial expiry but operations are silently
no-ops. Purchase a license to restore full functionality.

## Support

- Documentation: [atomik.tech](https://atomik.tech)
- Issues: [github.com/MatthewHRockwell/ATOMiK](https://github.com/MatthewHRockwell/ATOMiK/issues)
- Enterprise support: sales@atomik.tech
