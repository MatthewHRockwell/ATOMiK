VERSION := 0.5.0
KDIR ?= /lib/modules/$(shell uname -r)/build
DIST_NAME := atomik-$(VERSION)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(MAKE) -C tools clean
	rm -rf $(DIST_NAME) $(DIST_NAME).tar.gz

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

tools:
	$(MAKE) -C tools all

# Build a customer distribution tarball (no secrets, no keygen)
dist: clean
	mkdir -p $(DIST_NAME)/tools
	mkdir -p $(DIST_NAME)/include/uapi
	mkdir -p $(DIST_NAME)/systemd
	cp Kbuild Makefile dkms.conf install.sh uninstall.sh \
	   atomik_main.c atomik_chardev.c atomik_fingerprint.c \
	   atomik_sysfs.c atomik_hw.c atomik_hw.h atomik_license.c \
	   atomik_cow.c atomik_cow.h \
	   atomik_net.c atomik_net.h \
	   atomik_cgroup.c atomik_cgroup.h \
	   atomik_proc.c atomik_trace.h atomik_trace_define.c \
	   atomik_core_kern.h atomik_stats.h \
	   README.md .gitignore \
	   $(DIST_NAME)/
	cp include/uapi/atomik.h $(DIST_NAME)/include/uapi/
	cp tools/Makefile tools/atomik-status.c tools/atomik-test.c \
	   tools/atomik-bench.c tools/atomik-optimizer.c \
	   tools/atomik-dashboard.c tools/atomik-report.c \
	   tools/atomik-workload-bench.sh \
	   tools/atomik-full-test.sh \
	   $(DIST_NAME)/tools/
	cp systemd/atomik.service systemd/atomik.conf systemd/99-atomik.rules \
	   $(DIST_NAME)/systemd/
	tar czf $(DIST_NAME).tar.gz $(DIST_NAME)
	rm -rf $(DIST_NAME)
	@echo "Created $(DIST_NAME).tar.gz"
	@echo "Verify no secrets: tar tzf $(DIST_NAME).tar.gz | grep -i keygen"

.PHONY: all clean install tools dist
