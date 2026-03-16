#!/bin/bash
# ATOMiK Kernel Module — Container Entrypoint
# Handles build, load, run, and graceful cleanup lifecycle.
set -euo pipefail

KMOD_SRC="/opt/atomik"
MODULE_NAME="atomik"
KVER=$(uname -r)
KDIR="/lib/modules/${KVER}/build"

log() { echo "[atomik] $(date -u '+%Y-%m-%dT%H:%M:%SZ') $*"; }

# ─── Cleanup on SIGTERM/SIGINT ──────────────────────────────────────
cleanup() {
    log "Received shutdown signal"
    if lsmod | grep -q "^${MODULE_NAME} "; then
        log "Unloading ${MODULE_NAME} module..."
        rmmod "${MODULE_NAME}" && log "Module unloaded" || log "WARN: rmmod failed"
    fi
    exit 0
}
trap cleanup SIGTERM SIGINT

# ─── Pre-flight ─────────────────────────────────────────────────────
log "ATOMiK kernel module container starting"
log "Host kernel: ${KVER}"

if [ ! -d "/lib/modules/${KVER}" ]; then
    log "ERROR: /lib/modules/${KVER} not mounted from host"
    exit 1
fi

if [ ! -d "${KDIR}" ]; then
    log "ERROR: Kernel headers not found at ${KDIR}"
    log "Ensure the host has linux-headers-${KVER} installed"
    exit 1
fi

# ─── Build ──────────────────────────────────────────────────────────
log "Building module against kernel ${KVER}..."
cd "${KMOD_SRC}"
make -C "${KDIR}" M="${KMOD_SRC}" modules 2>&1 | while read -r line; do
    log "  build: ${line}"
done

if [ ! -f "${KMOD_SRC}/${MODULE_NAME}.ko" ]; then
    log "ERROR: Build failed — ${MODULE_NAME}.ko not found"
    exit 1
fi
log "Build complete: $(modinfo -F vermagic "${KMOD_SRC}/${MODULE_NAME}.ko" 2>/dev/null || echo 'unknown')"

# ─── Load ───────────────────────────────────────────────────────────
# Unload if already present (e.g., container restart)
if lsmod | grep -q "^${MODULE_NAME} "; then
    log "Module already loaded, removing first..."
    rmmod "${MODULE_NAME}" || true
fi

INSMOD_ARGS=""
if [ -n "${ATOMIK_LICENSE_KEY:-}" ]; then
    INSMOD_ARGS="license_key=${ATOMIK_LICENSE_KEY}"
    log "Loading module with license key"
else
    log "Loading module (community edition)"
fi

insmod "${KMOD_SRC}/${MODULE_NAME}.ko" ${INSMOD_ARGS}
log "Module loaded successfully"

# Verify the module is operational
if [ -d "/proc/atomik" ]; then
    log "  /proc/atomik: present"
fi
if [ -c "/dev/atomik" ]; then
    log "  /dev/atomik: present"
fi

# ─── Run ────────────────────────────────────────────────────────────
# If the Prometheus exporter exists, run it; otherwise sleep forever.
EXPORTER="${KMOD_SRC}/tools/atomik-exporter.py"
if [ -f "${EXPORTER}" ] && command -v python3 >/dev/null 2>&1; then
    PROM_PORT="${ATOMIK_PROM_PORT:-9471}"
    log "Starting Prometheus exporter on :${PROM_PORT}"
    exec python3 "${EXPORTER}" --port "${PROM_PORT}" &
    EXPORTER_PID=$!
    wait ${EXPORTER_PID} || true
else
    log "No exporter found, idling (module remains loaded)"
    # Sleep in a loop so the trap can fire on SIGTERM
    while true; do
        sleep 3600 &
        wait $! || break
    done
fi
