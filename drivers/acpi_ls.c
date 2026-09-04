/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * ACPI EC host I/F backend via LinkedSemi KCS (eSPI 0x60/0x64 on EVB).
 * Preserves acpi.h call surface used by smchost.
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "acpi.h"

#if defined(CONFIG_KCS) && DT_NODE_HAS_STATUS(DT_NODELABEL(e8042), okay)
#include <zephyr/drivers/kcs.h>
#define ACPI_HAS_HW_KCS 1
#else
#define ACPI_HAS_HW_KCS 0
#endif

LOG_MODULE_REGISTER(acpi_ls, CONFIG_SMCHOST_LOG_LEVEL);

static uint8_t soft_sts[2];
static uint8_t soft_idr[2];
static uint8_t soft_odr[2];

#if ACPI_HAS_HW_KCS
static const struct device *kcs_dev;
static bool use_hw_kcs;
#endif

static void acpi_ensure_dev(void)
{
	static bool probed;

	if (probed) {
		return;
	}
	probed = true;

#if ACPI_HAS_HW_KCS
	kcs_dev = DEVICE_DT_GET(DT_NODELABEL(e8042));
	if (device_is_ready(kcs_dev)) {
		use_hw_kcs = true;
		LOG_INF("ACPI via KCS %s", kcs_dev->name);
		return;
	}
#endif
	LOG_WRN("ACPI KCS unavailable — using soft registers");
}

bool acpi_get_flag(enum acpi_ec_interface num, uint8_t type)
{
	acpi_ensure_dev();

	if (num > ACPI_EC_1) {
		return false;
	}

#if ACPI_HAS_HW_KCS
	if (use_hw_kcs && num == ACPI_EC_0) {
		uint8_t sts = 0;

		if (kcs_read_status(kcs_dev, &sts) != 0) {
			return false;
		}
		if (type == ACPI_FLAG_OBF) {
			return (sts & KCS_OBF) != 0;
		}
		if (type == ACPI_FLAG_IBF) {
			return (sts & KCS_IBF) != 0;
		}
		if (type == ACPI_FLAG_CD) {
			return (sts & KCS_CMD_DAT) != 0;
		}
		return (sts & type) != 0;
	}
#endif

	return (soft_sts[num] & type) != 0;
}

void acpi_set_flag(enum acpi_ec_interface num, uint8_t type, bool val)
{
	acpi_ensure_dev();

	if (num > ACPI_EC_1) {
		return;
	}

#if ACPI_HAS_HW_KCS
	if (use_hw_kcs && num == ACPI_EC_0) {
		(void)kcs_update_status(kcs_dev, type, val ? type : 0);
		return;
	}
#endif

	if (val) {
		soft_sts[num] |= type;
	} else {
		soft_sts[num] &= ~type;
	}
}

uint8_t acpi_read_idr(enum acpi_ec_interface num)
{
	acpi_ensure_dev();

	if (num > ACPI_EC_1) {
		return 0;
	}

#if ACPI_HAS_HW_KCS
	if (use_hw_kcs && num == ACPI_EC_0) {
		uint8_t data = 0;

		if (kcs_read_data(kcs_dev, &data) != 0) {
			return 0;
		}
		return data;
	}
#endif

	return soft_idr[num];
}

void acpi_write_odr(enum acpi_ec_interface num, uint8_t byte)
{
	acpi_ensure_dev();

	if (num > ACPI_EC_1) {
		return;
	}

#if ACPI_HAS_HW_KCS
	if (use_hw_kcs && num == ACPI_EC_0) {
		(void)kcs_write_data(kcs_dev, byte);
		return;
	}
#endif

	soft_odr[num] = byte;
	soft_sts[num] |= ACPI_FLAG_OBF;
}

uint8_t acpi_read_str(enum acpi_ec_interface num)
{
	acpi_ensure_dev();

	if (num > ACPI_EC_1) {
		return 0;
	}

#if ACPI_HAS_HW_KCS
	if (use_hw_kcs && num == ACPI_EC_0) {
		uint8_t sts = 0;

		if (kcs_read_status(kcs_dev, &sts) != 0) {
			return 0;
		}
		return sts;
	}
#endif

	return soft_sts[num];
}

void acpi_write_str(enum acpi_ec_interface num, uint8_t byte)
{
	acpi_ensure_dev();

	if (num > ACPI_EC_1) {
		return;
	}

#if ACPI_HAS_HW_KCS
	if (use_hw_kcs && num == ACPI_EC_0) {
		(void)kcs_update_status(kcs_dev, 0xFF, byte);
		return;
	}
#endif

	soft_sts[num] = byte;
}

int acpi_send_byte(enum acpi_ec_interface num, uint8_t data)
{
	for (uint16_t i = 0; i < HOST_TIMEOUT; i++) {
		if (acpi_get_flag(num, ACPI_FLAG_OBF) == 1) {
			k_busy_wait(10);
			continue;
		}
		acpi_write_odr(num, data);
		return 0;
	}
	return -ETIMEDOUT;
}
