/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Fan backend for LS101x using single PWM + CAP (hwmon) where available.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include "gpio_ec.h"
#include "board_config.h"
#include "fan.h"

LOG_MODULE_REGISTER(fan, CONFIG_FAN_LOG_LEVEL);

#define MAX_DUTY_CYCLE		100u
#define FAN_PWM_PERIOD_NS	200000u /* 5 kHz */

static const struct device *pwm_dev;
static struct fan_dev *fan_table;
static int fan_count;
static uint8_t last_duty[FAN_DEV_TOTAL];

int fan_init(int size, struct fan_dev *fan_tbl)
{
	fan_table = fan_tbl;
	fan_count = size;

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm), okay)
	pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm));
	if (!device_is_ready(pwm_dev)) {
		LOG_ERR("PWM device not ready");
		return -ENODEV;
	}

	LOG_INF("fan_init: %d entries, pwm ready", size);
	return 0;
#else
	ARG_UNUSED(size);
	pwm_dev = NULL;
	LOG_WRN("fan_init: pwm node disabled (pin quarantine)");
	return -ENODEV;
#endif
}

int fan_power_set(bool power_state)
{
	ARG_UNUSED(power_state);
#ifdef FAN_PWR_DISABLE_N
	return gpio_write_pin(FAN_PWR_DISABLE_N, power_state ? 1 : 0);
#else
	return 0;
#endif
}

int fan_set_duty_cycle(enum fan_type fan_idx, uint8_t duty_cycle)
{
	uint32_t pulse;
	int ch;
	int ret;

	if (fan_idx >= FAN_DEV_TOTAL || !fan_table) {
		return -EINVAL;
	}

	if (duty_cycle > MAX_DUTY_CYCLE) {
		duty_cycle = MAX_DUTY_CYCLE;
	}

	if (fan_table[fan_idx].pwm_ch == PWM_CH_UNDEF) {
		return -ENODEV;
	}

	if (pwm_dev == NULL || !device_is_ready(pwm_dev)) {
		return -ENODEV;
	}

	ch = (int)fan_table[fan_idx].pwm_ch;
	pulse = (FAN_PWM_PERIOD_NS * duty_cycle) / MAX_DUTY_CYCLE;
	ret = pwm_set(pwm_dev, ch, FAN_PWM_PERIOD_NS, pulse, 0);
	if (ret) {
		LOG_ERR("pwm_set ch %d failed %d", ch, ret);
		return ret;
	}

	last_duty[fan_idx] = duty_cycle;
	LOG_DBG("fan %d duty %u%%", fan_idx, duty_cycle);
	return 0;
}

int fan_read_rpm(enum fan_type fan_idx, uint16_t *rpm)
{
	if (!rpm || fan_idx >= FAN_DEV_TOTAL) {
		return -EINVAL;
	}

	/* CAP/hwmon wiring varies by board — PoC returns synthetic RPM */
	*rpm = (uint16_t)(last_duty[fan_idx] * 30);
	return 0;
}
