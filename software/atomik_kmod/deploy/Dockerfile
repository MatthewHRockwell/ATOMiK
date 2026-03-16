# ATOMiK Kernel Module — Container Image
# Builds and loads the ATOMiK delta-state accelerator kernel module
# on the host kernel at container startup.
#
# Requires privileged mode and /lib/modules mounted from the host.

FROM ubuntu:24.04

LABEL maintainer="ATOMiK <hello@atomik.dev>"
LABEL description="ATOMiK delta-state accelerator kernel module"
LABEL version="0.5.0"

# Avoid interactive prompts during package install
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies — kept minimal for image size
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        kmod \
        python3 \
    && rm -rf /var/lib/apt/lists/*

# Copy kernel module source (no build artifacts, no secrets)
COPY Kbuild Makefile dkms.conf /opt/atomik/
COPY atomik_main.c atomik_chardev.c atomik_fingerprint.c \
     atomik_sysfs.c atomik_hw.c atomik_license.c \
     atomik_cow.c atomik_net.c atomik_cgroup.c \
     atomik_proc.c atomik_trace_define.c \
     atomik_core_kern.h atomik_stats.h \
     atomik_hw.h atomik_cow.h atomik_net.h \
     atomik_cgroup.h atomik_trace.h \
     /opt/atomik/
COPY include/ /opt/atomik/include/
COPY tools/ /opt/atomik/tools/

# Copy entrypoint
COPY deploy/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Prometheus exporter port
EXPOSE 9471

ENTRYPOINT ["/entrypoint.sh"]
