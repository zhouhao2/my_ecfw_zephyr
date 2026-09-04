/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * gpio_ec backend for LS101x (gpioa..gpioh).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/logging/log.h>
#include "gpio_ec.h"

LOG_MODULE_REGISTER(gpio_ec, CONFIG_GPIO_EC_LOG_LEVEL);

#define MAX_PINS_PER_PORT	16

static const struct device *ports[] = {
	DEVICE_DT_GET(DT_NODELABEL(gpioa)),
	DEVICE_DT_GET(DT_NODELABEL(gpiob)),
	DEVICE_DT_GET(DT_NODELABEL(gpioc)),
	DEVICE_DT_GET(DT_NODELABEL(gpiod)),
	DEVICE_DT_GET(DT_NODELABEL(gpioe)),
	DEVICE_DT_GET(DT_NODELABEL(gpiof)),
	DEVICE_DT_GET(DT_NODELABEL(gpiog)),
	DEVICE_DT_GET(DT_NODELABEL(gpioh)),
};

struct gpio_port_pin {
	const struct device *gpio_dev;
	gpio_pin_t pin;
};

uint32_t get_absolute_gpio_num(uint32_t port_pin)
{
	return (MAX_PINS_PER_PORT * gpio_get_port(port_pin) + gpio_get_pin(port_pin));
}

static int validate_device(uint32_t port_pin, struct gpio_port_pin *pp)
{
	uint32_t port_idx = gpio_get_port(port_pin);
	gpio_pin_t pin = gpio_get_pin(port_pin);

	if (port_idx >= ARRAY_SIZE(ports)) {
		return -EINVAL;
	}

	if (!device_is_ready(ports[port_idx])) {
		LOG_ERR("gpio port %u not ready", port_idx);
		return -ENODEV;
	}

	pp->gpio_dev = ports[port_idx];
	pp->pin = pin;
	return 0;
}

static bool is_dummy_gpio(uint32_t port_pin)
{
	return gpio_get_port(port_pin) == EC_DUMMY_GPIO_PORT;
}

static bool gpio_read_dummy_pin(uint32_t port_pin)
{
	return gpio_get_pin(port_pin);
}

int gpio_init(void)
{
	for (int i = 0; i < ARRAY_SIZE(ports); i++) {
		if (!device_is_ready(ports[i])) {
			LOG_ERR("GPIO port %d not ready", i);
		} else {
			LOG_DBG("[Port %c] ready", 'A' + i);
		}
	}
	return 0;
}

int gpio_configure_pin(uint32_t port_pin, gpio_flags_t flags)
{
	int ret;
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return 0;
	}

	ret = validate_device(port_pin, &pp);
	if (ret) {
		return ret;
	}

	return gpio_pin_configure(pp.gpio_dev, pp.pin, flags);
}

int gpio_configure_array(struct gpio_ec_config *gpios, uint32_t len)
{
	for (uint32_t i = 0; i < len; i++) {
		int ret;

		if (is_dummy_gpio(gpios[i].port_pin)) {
			continue;
		}

		ret = gpio_configure_pin(gpios[i].port_pin, gpios[i].cfg);
		if (ret) {
			LOG_ERR("Config fail idx %u ret %d", i, ret);
		}
	}
	return 0;
}

int gpio_write_pin(uint32_t port_pin, int value)
{
	int ret;
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return 0;
	}

	ret = validate_device(port_pin, &pp);
	if (ret) {
		return ret;
	}

	return gpio_pin_set_raw(pp.gpio_dev, pp.pin, value);
}

int gpio_read_pin(uint32_t port_pin)
{
	int ret;
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return gpio_read_dummy_pin(port_pin);
	}

	ret = validate_device(port_pin, &pp);
	if (ret) {
		return ret;
	}

	return gpio_pin_get_raw(pp.gpio_dev, pp.pin);
}

int gpio_init_callback_pin(uint32_t port_pin,
			   struct gpio_callback *callback,
			   gpio_callback_handler_t handler)
{
	int ret;
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return 0;
	}

	ret = validate_device(port_pin, &pp);
	if (ret) {
		return ret;
	}

	gpio_init_callback(callback, handler, BIT(pp.pin));
	return 0;
}

int gpio_add_callback_pin(uint32_t port_pin, struct gpio_callback *callback)
{
	int ret;
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return 0;
	}

	ret = validate_device(port_pin, &pp);
	if (ret) {
		return ret;
	}

	return gpio_add_callback(pp.gpio_dev, callback);
}

int gpio_remove_callback_pin(uint32_t port_pin, struct gpio_callback *callback)
{
	int ret;
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return 0;
	}

	ret = validate_device(port_pin, &pp);
	if (ret) {
		return ret;
	}

	return gpio_remove_callback(pp.gpio_dev, callback);
}

int gpio_interrupt_configure_pin(uint32_t port_pin, gpio_flags_t flags)
{
	int ret;
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return 0;
	}

	ret = validate_device(port_pin, &pp);
	if (ret) {
		return ret;
	}

	return gpio_pin_interrupt_configure(pp.gpio_dev, pp.pin, flags);
}

bool gpio_port_enabled(uint32_t port_pin)
{
	struct gpio_port_pin pp;

	if (is_dummy_gpio(port_pin)) {
		return false;
	}

	return validate_device(port_pin, &pp) == 0;
}

int gpio_force_configure_pin(uint32_t port_pin, gpio_flags_t flags)
{
	/* No SAF pinmux override on LS PoC — same as configure */
	return gpio_configure_pin(port_pin, flags);
}
