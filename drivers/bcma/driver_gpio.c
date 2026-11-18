/*
 * Broadcom specific AMBA
 * GPIO driver
 *
 * Copyright 2011, Broadcom Corporation
 * Copyright 2012, Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Licensed under the GNU/GPL. See COPYING for details.
 */

#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/export.h>
#include <linux/property.h>

#include <linux/bcma/bcma.h>

#include "bcma_private.h"

#define BCMA_GPIO_MAX_PINS	32

const struct software_node bcma_gpio_swnode = {
	.name = "bcma-gpio",
};
EXPORT_SYMBOL_GPL(bcma_gpio_swnode);

static int bcma_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct bcma_drv_cc *cc = gpiochip_get_data(chip);

	return !!bcma_chipco_gpio_in(cc, 1 << gpio);
}

static int bcma_gpio_set_value(struct gpio_chip *chip, unsigned int gpio,
			       int value)
{
	struct bcma_drv_cc *cc = gpiochip_get_data(chip);

	bcma_chipco_gpio_out(cc, 1 << gpio, value ? 1 << gpio : 0);

	return 0;
}

static int bcma_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	struct bcma_drv_cc *cc = gpiochip_get_data(chip);

	bcma_chipco_gpio_outen(cc, 1 << gpio, 0);
	return 0;
}

static int bcma_gpio_direction_output(struct gpio_chip *chip, unsigned gpio,
				      int value)
{
	struct bcma_drv_cc *cc = gpiochip_get_data(chip);

	bcma_chipco_gpio_outen(cc, 1 << gpio, 1 << gpio);
	bcma_chipco_gpio_out(cc, 1 << gpio, value ? 1 << gpio : 0);
	return 0;
}

static int bcma_gpio_request(struct gpio_chip *chip, unsigned gpio)
{
	struct bcma_drv_cc *cc = gpiochip_get_data(chip);

	bcma_chipco_gpio_control(cc, 1 << gpio, 0);
	/* clear pulldown */
	bcma_chipco_gpio_pulldown(cc, 1 << gpio, 0);
	/* Set pullup */
	bcma_chipco_gpio_pullup(cc, 1 << gpio, 1 << gpio);

	return 0;
}

static void bcma_gpio_free(struct gpio_chip *chip, unsigned gpio)
{
	struct bcma_drv_cc *cc = gpiochip_get_data(chip);

	/* clear pullup */
	bcma_chipco_gpio_pullup(cc, 1 << gpio, 0);
}

static int bcma_gpio_irq_init(struct bcma_drv_cc *cc)
{
	return 0;
}

static void bcma_gpio_irq_exit(struct bcma_drv_cc *cc)
{
}

int bcma_gpio_init(struct bcma_drv_cc *cc)
{
	struct bcma_bus *bus = cc->core->bus;
	struct gpio_chip *chip = &cc->gpio;
	int err;

	chip->label		= "bcma_gpio";
	chip->owner		= THIS_MODULE;
	chip->request		= bcma_gpio_request;
	chip->free		= bcma_gpio_free;
	chip->get		= bcma_gpio_get_value;
	chip->set		= bcma_gpio_set_value;
	chip->direction_input	= bcma_gpio_direction_input;
	chip->direction_output	= bcma_gpio_direction_output;
	chip->parent		= bus->dev;

	/*
	 * Register software node only for the host SoC bus, unless there is
	 * already a firmware node assigned. There is only one SoC instance
	 * in the system, so there are no concerns with registration conflicts.
	 */
	if (bus->hosttype == BCMA_HOSTTYPE_SOC && !dev_fwnode(&cc->core->dev)) {
		err = software_node_register(&bcma_gpio_swnode);
		if (err)
			return err;
		chip->fwnode = software_node_fwnode(&bcma_gpio_swnode);
	} else {
		chip->fwnode = dev_fwnode(&cc->core->dev);
	}

	switch (bus->chipinfo.id) {
	case BCMA_CHIP_ID_BCM4707:
	case BCMA_CHIP_ID_BCM5357:
	case BCMA_CHIP_ID_BCM53572:
	case BCMA_CHIP_ID_BCM53573:
	case BCMA_CHIP_ID_BCM47094:
		chip->ngpio	= 32;
		break;
	default:
		chip->ngpio	= 16;
	}

	/*
	 * Register SoC GPIO devices with absolute GPIO pin base.
	 * On MIPS, we don't have Device Tree and we can't use relative (per chip)
	 * GPIO numbers.
	 * On some ARM devices, user space may want to access some system GPIO
	 * pins directly, which is easier to do with a predictable GPIO base.
	 */
	if (IS_BUILTIN(CONFIG_BCM47XX) ||
	    cc->core->bus->hosttype == BCMA_HOSTTYPE_SOC)
		chip->base		= bus->num * BCMA_GPIO_MAX_PINS;
	else
		chip->base		= -1;

	err = bcma_gpio_irq_init(cc);
	if (err)
		goto err_unregister_swnode;

	err = gpiochip_add_data(chip, cc);
	if (err)
		goto err_irq_exit;

	return 0;

err_irq_exit:
	bcma_gpio_irq_exit(cc);
err_unregister_swnode:
	if (bus->hosttype == BCMA_HOSTTYPE_SOC &&
	    chip->fwnode && is_software_node(chip->fwnode)) {
		software_node_unregister(&bcma_gpio_swnode);
		chip->fwnode = NULL;
	}
	return err;
}

int bcma_gpio_unregister(struct bcma_drv_cc *cc)
{
	bcma_gpio_irq_exit(cc);
	gpiochip_remove(&cc->gpio);
	if (cc->core->bus->hosttype == BCMA_HOSTTYPE_SOC &&
	    cc->gpio.fwnode && is_software_node(cc->gpio.fwnode)) {
		software_node_unregister(&bcma_gpio_swnode);
		cc->gpio.fwnode = NULL;
	}
	return 0;
}
