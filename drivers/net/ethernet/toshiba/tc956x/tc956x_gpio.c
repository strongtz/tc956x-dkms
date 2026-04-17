// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

/*
 * tc956x_gpio.c - Minimal GPIO controller driver for  QPS615 PHY reset lines.
*/

#include <linux/version.h>
#include <linux/gpio/driver.h>

#include "tc956xmac.h"

#define TC956X_GPIO_COUNT	14

struct tc956x_gpio {
	struct gpio_chip	 chip;
	struct tc956xmac_priv	*priv;
};

static inline struct tc956x_gpio *to_tc956x_gpio(struct gpio_chip *chip)
{
	return gpiochip_get_data(chip);
}

static int tc956x_gpio_direction_output(struct gpio_chip *chip,
					unsigned int offset, int value)
{
	return tc956x_GPIO_OutputConfigPin(to_tc956x_gpio(chip)->priv,
					   offset, value ? 1 : 0);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)
static void tc956x_gpio_set(struct gpio_chip *chip, unsigned int offset,
			    int value)
{
    tc956x_GPIO_OutputConfigPin(to_tc956x_gpio(chip)->priv,
					   offset, value ? 1 : 0);
	return;
}
#else
static int tc956x_gpio_set(struct gpio_chip *chip, unsigned int offset,
			    int value)
{
	return tc956x_GPIO_OutputConfigPin(to_tc956x_gpio(chip)->priv,
					   offset, value ? 1 : 0);
}
#endif

int tc956x_gpio_register(struct tc956xmac_priv *priv)
{
	struct tc956x_gpio *tgpio;

	tgpio = devm_kzalloc(priv->device, sizeof(*tgpio), GFP_KERNEL);
	if (!tgpio)
		return -ENOMEM;

	tgpio->priv			= priv;
	tgpio->chip.label		= "tc956x-gpio";
	tgpio->chip.parent		= priv->device;
	tgpio->chip.owner		= THIS_MODULE;
	tgpio->chip.direction_output	= tc956x_gpio_direction_output;
	tgpio->chip.set			= tc956x_gpio_set;
	tgpio->chip.base		= -1;
	tgpio->chip.ngpio		= TC956X_GPIO_COUNT;
	tgpio->chip.can_sleep		= false;

	return devm_gpiochip_add_data(priv->device, &tgpio->chip, tgpio);
}
