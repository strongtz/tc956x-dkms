// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2021, The Linux Foundation. All rights reserved.
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>

#include "tc956xmac.h"

struct tc956x_qcom_priv {
	struct gpio_desc	*phy_reset_gpio;
	u32			 phy_reset_post_delay_us;
	struct regulator	*phy_supply;
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pinctrl_default;
	int			 wol_irq;
};

#define to_qpriv(priv)	((struct tc956x_qcom_priv *)(priv)->plat_priv)

static int tc956x_phy_reset_assert(struct tc956xmac_priv *priv)
{
	if (!to_qpriv(priv)->phy_reset_gpio)
		return 0;

	gpiod_set_value_cansleep(to_qpriv(priv)->phy_reset_gpio, 1);
	return 0;
}

static int tc956x_phy_reset_deassert(struct tc956xmac_priv *priv)
{
	if (!to_qpriv(priv)->phy_reset_gpio)
		return 0;

	gpiod_set_value_cansleep(to_qpriv(priv)->phy_reset_gpio, 0);
	return 0;
}

static int tc956x_phy_power_on(struct tc956xmac_priv *priv)
{
	struct tc956x_qcom_priv *qpriv = to_qpriv(priv);
	int ret;

	if (qpriv->phy_supply) {
		ret = regulator_enable(qpriv->phy_supply);
		if (ret) {
			dev_err(priv->device,
				"Failed to enable PHY supply: %d\n", ret);
			return ret;
		}
	}

	ret = tc956x_phy_reset_deassert(priv);
	if (ret) {
		dev_err(priv->device,
			"Failed to deassert PHY reset: %d\n", ret);
		if (qpriv->phy_supply)
			regulator_disable(qpriv->phy_supply);
		return ret;
	}

	fsleep(qpriv->phy_reset_post_delay_us);
	return 0;
}

static int tc956x_phy_power_off(struct tc956xmac_priv *priv)
{
	struct tc956x_qcom_priv *qpriv = to_qpriv(priv);
	int ret;

	ret = tc956x_phy_reset_assert(priv);
	if (ret) {
		dev_err(priv->device,
			"Failed to assert PHY reset: %d\n", ret);
		return ret;
	}

	if (qpriv->phy_supply) {
		ret = regulator_disable(qpriv->phy_supply);
		if (ret)
			dev_err(priv->device,
				"Failed to disable PHY supply: %d\n", ret);
	}

	return ret;
}

int tc956x_platform_port_interface_overlay(struct device *dev,
					   struct tc956xmac_resources *res)
{
	const char *phy_mode;

	if (!dev->of_node)
		return 0;

	if (of_property_read_string(dev->of_node, "phy-mode", &phy_mode) &&
	    of_property_read_string(dev->of_node, "phy-connection-type", &phy_mode))
		return 0;

	if (!strcmp(phy_mode, "usxgmii")) {
		res->port_interface = ENABLE_USXGMII_10G_INTERFACE;
	} else if (!strcmp(phy_mode, "10gbase-r")) {
		res->port_interface = ENABLE_XFI_INTERFACE;
	} else if (!strcmp(phy_mode, "sgmii")) {
		res->port_interface = ENABLE_SGMII_INTERFACE;
	} else if (!strcmp(phy_mode, "2500base-x")) {
		res->port_interface = ENABLE_2500BASE_X_INTERFACE;
	} else if (!strcmp(phy_mode, "rgmii")) {
		res->port_interface = ENABLE_RGMII_INTERFACE;
	} else if (!strcmp(phy_mode, "rgmii-id")) {
		res->port_interface = ENABLE_RGMII_ID_INTERFACE;
	} else {
		dev_warn(dev, "Unsupported phy-mode %s for TC956x interface overlay\n",
			 phy_mode);
		return 0;
	}

	dev_info(dev, "TC956x interface overlay from phy-mode %s: %u\n",
		 phy_mode, res->port_interface);

	return 1;
}

static int tc956x_platform_of_parse(struct device *dev,
				    struct tc956x_qcom_priv *qpriv)
{
	int ret;

	qpriv->phy_reset_gpio = devm_gpiod_get_optional(dev, "phy-reset",
							GPIOD_OUT_HIGH);
	if (IS_ERR(qpriv->phy_reset_gpio))
		return dev_err_probe(dev, PTR_ERR(qpriv->phy_reset_gpio),
				     "Failed to get phy-reset-gpios\n");

	of_property_read_u32(dev->of_node, "reset-deassert-us",
			     &qpriv->phy_reset_post_delay_us);

	/*
	 * Some boards have an always-on supply, so this is optional.
	 */
	qpriv->phy_supply = devm_regulator_get_optional(dev, "phy");
	if (IS_ERR(qpriv->phy_supply)) {
		ret = PTR_ERR(qpriv->phy_supply);
		if (ret != -ENODEV)
			return dev_err_probe(dev, ret,
					     "Failed to get phy-supply\n");
		qpriv->phy_supply = NULL;
	}

	/* wol_irq is optional */
	qpriv->wol_irq = of_irq_get_byname(dev->of_node, "wol_irq");
	if (qpriv->wol_irq < 0) {
		ret = qpriv->wol_irq;
		if (ret == -EPROBE_DEFER)
			return ret;
		dev_dbg(dev, "No wol_irq defined, WoL IRQ disabled\n");
		qpriv->wol_irq = 0;
	}

	/* pinctrl is optional */
	qpriv->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(qpriv->pinctrl)) {
		ret = PTR_ERR(qpriv->pinctrl);
		if (ret != -ENODEV)
			return dev_err_probe(dev, ret,
					     "Failed to get pinctrl\n");
		qpriv->pinctrl = NULL;
	}

	if (qpriv->pinctrl) {
		qpriv->pinctrl_default =
			pinctrl_lookup_state(qpriv->pinctrl,
					     PINCTRL_STATE_DEFAULT);
		if (IS_ERR(qpriv->pinctrl_default))
			return dev_err_probe(dev,
					     PTR_ERR(qpriv->pinctrl_default),
					     "Failed to look up default pinctrl state\n");
	}

	return 0;
}

int tc956x_platform_probe(struct tc956xmac_priv *priv,
			  struct tc956xmac_resources *res)
{
	struct tc956x_qcom_priv *qpriv;
	int ret;

	qpriv = devm_kzalloc(priv->device, sizeof(*qpriv), GFP_KERNEL);
	if (!qpriv)
		return -ENOMEM;

	priv->plat_priv = qpriv;

	/*
	 * Register the TC956X GPIO controller.
	 * Ensure that only port 0 owns the GPIO hardware.
	 */
	if (priv->port_num == RM_PF0_ID) {
		ret = tc956x_gpio_register(priv);
		if (ret) {
			dev_err(priv->device,
				"Failed to register GPIO controller: %d\n",
				ret);
			goto err_out;
		}
	}

	ret = tc956x_platform_of_parse(priv->device, qpriv);
	if (ret)
		goto err_out;

	if (qpriv->pinctrl) {
		ret = pinctrl_select_state(qpriv->pinctrl,
					   qpriv->pinctrl_default);
		if (ret) {
			dev_err(priv->device,
				"Failed to select default pinctrl state: %d\n",
				ret);
			goto err_out;
		}
	}

	ret = tc956x_phy_power_on(priv);
	if (ret) {
		dev_err(priv->device, "Failed to power on PHY: %d\n", ret);
		goto err_out;
	}

	res->wol_irq = qpriv->wol_irq;
	return 0;

err_out:
	priv->plat_priv = NULL;
	return ret;
}

int tc956x_platform_remove(struct tc956xmac_priv *priv)
{
	int ret;

	ret = tc956x_phy_power_off(priv);
	if (ret)
		dev_err(priv->device, "Failed to power off PHY: %d\n", ret);

	priv->plat_priv = NULL;
	return ret;
}

int tc956x_platform_suspend(struct tc956xmac_priv *priv)
{
	int ret = 0;

	if (priv->wolopts) {
		ret = enable_irq_wake(priv->wol_irq);
		if (ret)
			dev_err(priv->device,
				"Failed to enable WoL IRQ %d as wakeup source: %d\n",
				priv->wol_irq, ret);
	} else {
		ret = tc956x_phy_power_off(priv);
		if (ret)
			dev_err(priv->device,
				"Failed to power off PHY: %d\n", ret);
	}

	return ret;
}

int tc956x_platform_resume(struct tc956xmac_priv *priv)
{
	int ret = 0;

	if (priv->wolopts) {
		ret = disable_irq_wake(priv->wol_irq);
		if (ret)
			dev_err(priv->device,
				"Failed to disable WoL IRQ %d wakeup: %d\n",
				priv->wol_irq, ret);
	} else {
		ret = tc956x_phy_power_on(priv);
		if (ret)
			dev_err(priv->device,
				"Failed to power on PHY: %d\n", ret);
	}

	return ret;
}
