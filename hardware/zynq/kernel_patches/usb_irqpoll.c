// SPDX-License-Identifier: GPL-2.0
/*
 * usb_irqpoll — continuously service the PS USB0 controller IRQ via an hrtimer.
 *
 * On this AX7020 SoC (NaxRiscv RV64 in the PL) the PS->PL fabric interrupt tap
 * IRQ_P2F_USB0 does not reliably carry the chipidea controller's interrupt to
 * the PLIC: the controller asserts (USBSTS & USBINTR != 0) but PLIC source 3
 * never pends.  Booting with `irqpoll` polls the handler off the timer tick and
 * gets USB HID alive, but irqpoll is a spurious-IRQ *recovery* mechanism, not a
 * steady poller — it backs off after a second or two, so mouse/keyboard delivery
 * dies mid-use.  A PL-side poller (gateware reading USBSTS) delivered reliably
 * but broke USB enumeration as a third bus master.
 *
 * This is the software analogue with neither drawback: a 1 ms hrtimer forces the
 * USB IRQ handler to run continuously, independent of the broken fabric tap and
 * with no gateware bus master.  generic_handle_irq() runs the registered
 * ci_hdrc/ehci handler, which reads USBSTS, reaps completions, and resubmits —
 * keeping HID URBs alive.  Re-entrancy with a real IRQ is guarded by the flow
 * handler (IRQD_IRQ_INPROGRESS); the PLIC silently ignores a completion for a
 * source it didn't claim, so the trailing EOI is harmless.
 */
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqnr.h>
#include <linux/ktime.h>
#include <linux/string.h>
#include <linux/printk.h>

#define USBPOLL_NS   (1 * 1000 * 1000)   /* 1 ms — 8x oversamples HID's ~8 ms */
#define USBPOLL_NAME "80002000.usb"

static struct hrtimer usbpoll_timer;
static int usbpoll_virq = -1;

/* Find the virq whose action name is the USB controller.  Lazy: the controller
 * is probed during boot, possibly after this initcall, so resolve on the timer
 * path until found, then cache. */
static int usbpoll_find(void)
{
	struct irq_desc *desc;
	struct irqaction *act;
	int i;

	for_each_irq_desc(i, desc) {
		if (!desc)
			continue;
		for (act = desc->action; act; act = act->next) {
			if (act->name && strstr(act->name, USBPOLL_NAME))
				return i;
		}
	}
	return -1;
}

static enum hrtimer_restart usbpoll_fn(struct hrtimer *t)
{
	if (usbpoll_virq <= 0) {
		usbpoll_virq = usbpoll_find();
		if (usbpoll_virq > 0)
			pr_info("usb_irqpoll: servicing virq %d (%s) every 1ms\n",
				usbpoll_virq, USBPOLL_NAME);
	}
	if (usbpoll_virq > 0)
		generic_handle_irq(usbpoll_virq);

	hrtimer_forward_now(t, ns_to_ktime(USBPOLL_NS));
	return HRTIMER_RESTART;
}

static int __init usbpoll_init(void)
{
	pr_info("usb_irqpoll: starting 1ms USB IRQ poller\n");
	hrtimer_init(&usbpoll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	usbpoll_timer.function = usbpoll_fn;
	hrtimer_start(&usbpoll_timer, ns_to_ktime(USBPOLL_NS), HRTIMER_MODE_REL);
	return 0;
}
late_initcall(usbpoll_init);
