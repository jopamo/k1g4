// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for the 98626/98644/internal serial interface on hp300/hp400
 * (based on the National Semiconductor INS8250/NS16550AF/WD16C552 UARTs)
 *
 * Ported from 2.2 and modified to use the normal 8250 driver
 * by Kars de Jong <jongk@linux-m68k.org>, May 2004.
 */
#include <linux/module.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/delay.h>
#include <linux/dio.h>
#include <linux/console.h>
#include <linux/slab.h>
#include <asm/io.h>

#include "8250.h"

#if !defined(CONFIG_COMPILE_TEST)
#warning CONFIG_SERIAL_8250 defined but neither CONFIG_HPDCA nor CONFIG_HPAPCI defined, are you sure?
#endif


static unsigned int num_ports;

extern int hp300_uart_scode;

/* Offset to UART registers from base of DCA */
#define UART_OFFSET	17

#define DCA_ID		0x01	/* ID (read), reset (write) */
#define DCA_IC		0x03	/* Interrupt control        */

/* Interrupt control */
#define DCA_IC_IE	0x80	/* Master interrupt enable  */

#define HPDCA_BAUD_BASE 153600

/* Base address of the Frodo part */
#define FRODO_BASE	(0x41c000)

/*
 * Where we find the 8250-like APCI ports, and how far apart they are.
 */
#define FRODO_APCIBASE		0x0
#define FRODO_APCISPACE		0x20
#define FRODO_APCI_OFFSET(x)	(FRODO_APCIBASE + ((x) * FRODO_APCISPACE))

#define HPAPCI_BAUD_BASE 500400

#ifdef CONFIG_SERIAL_8250_CONSOLE
/*
 * Parse the bootinfo to find descriptions for headless console and
 * debug serial ports and register them with the 8250 driver.
 */
int __init hp300_setup_serial_console(void)
{
	int scode;
	struct uart_port port;

	memset(&port, 0, sizeof(port));

	if (hp300_uart_scode < 0 || hp300_uart_scode > DIO_SCMAX)
		return 0;

	if (DIO_SCINHOLE(hp300_uart_scode))
		return 0;

	scode = hp300_uart_scode;

	/* Memory mapped I/O */
	port.iotype = UPIO_MEM;
	port.flags = UPF_SKIP_TEST | UPF_SHARE_IRQ | UPF_BOOT_AUTOCONF;
	port.type = PORT_UNKNOWN;

	/* Check for APCI console */
	if (scode == 256) {
		pr_warn("Serial console is APCI but support is disabled (CONFIG_HPAPCI)!\n");
		return 0;
	} else {
		pr_warn("Serial console is DCA but support is disabled (CONFIG_HPDCA)!\n");
		return 0;
	}

	if (early_serial_setup(&port) < 0)
		pr_warn("%s: early_serial_setup() failed.\n", __func__);
	return 0;
}
#endif /* CONFIG_SERIAL_8250_CONSOLE */


static int __init hp300_8250_init(void)
{
	static int called;
	if (called)
		return -ENODEV;
	called = 1;

	if (!MACH_IS_HP300)
		return -ENODEV;


	/* Any boards found? */
	if (!num_ports)
		return -ENODEV;

	return 0;
}

static void __exit hp300_8250_exit(void)
{
}

module_init(hp300_8250_init);
module_exit(hp300_8250_exit);
MODULE_DESCRIPTION("HP DCA/APCI serial driver");
MODULE_AUTHOR("Kars de Jong <jongk@linux-m68k.org>");
MODULE_LICENSE("GPL");
