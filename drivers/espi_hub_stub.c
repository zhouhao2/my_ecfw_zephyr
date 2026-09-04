/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Software stub eSPI hub when CONFIG_ESPI is disabled.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "espi_hub.h"
#include "board_config.h"
#include "gpio_ec.h"

LOG_MODULE_REGISTER(espihub, CONFIG_ESPIHUB_LOG_LEVEL);

static struct espihub_context hub;
static espi_warn_handler_t warn_handlers[ESPIHUB_MAX_HANDLER_INDEX];
static espi_state_handler_t state_handler;
static espi_acpi_handler_t acpi_handlers[MAX_ACPI_HANDLERS];
static espi_kbc_handler_t kbc_handler;
static espi_postcode_handler_t postcode_handler;

int espihub_add_state_handler(espi_state_handler_t handler)
{
	if (state_handler) {
		return -EINVAL;
	}
	state_handler = handler;
	return 0;
}

int espihub_add_warn_handler(enum espihub_handler type,
			     espi_warn_handler_t handler)
{
	if (type >= ESPIHUB_MAX_HANDLER_INDEX || warn_handlers[type]) {
		return -EINVAL;
	}
	warn_handlers[type] = handler;
	return 0;
}

int espihub_add_acpi_handler(enum espihub_acpi_handler type,
			     espi_acpi_handler_t handler)
{
	if (type >= MAX_ACPI_HANDLERS || acpi_handlers[type]) {
		return -EINVAL;
	}
	acpi_handlers[type] = handler;
	return 0;
}

int espihub_add_kbc_handler(espi_kbc_handler_t handler)
{
	kbc_handler = handler;
	return 0;
}

int espihub_add_postcode_handler(espi_postcode_handler_t handler)
{
	postcode_handler = handler;
	return 0;
}

void detect_boot_mode(void)
{
	hub.spi_boot_mode = FLASH_BOOT_MODE_OWN;
	hub.boot_config_detected = true;
}

int espihub_init(void)
{
	hub.host_vw_ready = true;
	hub.espi_rst_sts = 1;
	hub.spi_boot_mode = FLASH_BOOT_MODE_OWN;
	hub.boot_config_detected = true;
	LOG_WRN("espihub stub active (CONFIG_ESPI=n)");
	return 0;
}

enum boot_config_mode espihub_boot_mode(void)
{
	return hub.spi_boot_mode;
}

bool espihub_reset_status(void)
{
	return hub.espi_rst_sts != 0;
}

bool espihub_dnx_status(void)
{
	return hub.dnx_mode;
}

int espihub_wait_for_espi_reset(uint8_t exp_sts, uint16_t timeout)
{
	ARG_UNUSED(timeout);
	hub.espi_rst_sts = exp_sts;
	return 0;
}

int espihub_wait_for_vwire(enum espi_vwire_signal signal, uint16_t timeout,
			   uint8_t exp_level, bool ack_required)
{
	ARG_UNUSED(signal);
	ARG_UNUSED(timeout);
	ARG_UNUSED(exp_level);
	ARG_UNUSED(ack_required);
	return 0;
}

int wait_for_pin_monitor_vwire(uint32_t port_pin, uint32_t exp_sts,
			       uint16_t timeout, enum espi_vwire_signal signal,
			       uint8_t abort_sts)
{
	ARG_UNUSED(port_pin);
	ARG_UNUSED(exp_sts);
	ARG_UNUSED(timeout);
	ARG_UNUSED(signal);
	ARG_UNUSED(abort_sts);
	return 0;
}

int espihub_send_vw(enum espi_vwire_signal signal, uint8_t level)
{
	LOG_INF("stub VW send sig=%d level=%u", signal, level);
	return 0;
}

int espihub_retrieve_vw(enum espi_vwire_signal signal, uint8_t *level)
{
	ARG_UNUSED(signal);
	if (level) {
		*level = 1;
	}
	return 0;
}

int espihub_retrieve_oob(struct espi_oob_packet *pckt)
{
	ARG_UNUSED(pckt);
	return -ENOTSUP;
}

int espihub_send_oob(struct espi_oob_packet *pckt)
{
	ARG_UNUSED(pckt);
	return -ENOTSUP;
}

int espihub_kbc_write(enum lpc_peripheral_opcode cmd, uint32_t payload)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(payload);
	return -ENOTSUP;
}

int espihub_kbc_read(enum lpc_peripheral_opcode cmd, uint32_t *data)
{
	ARG_UNUSED(cmd);
	if (data) {
		*data = 0;
	}
	return -ENOTSUP;
}

int espihub_send_ltr(void)
{
	return -ENOTSUP;
}

int espihub_write_flash(struct espi_flash_packet *pckt)
{
	ARG_UNUSED(pckt);
	return -ENOTSUP;
}

int espihub_read_flash(struct espi_flash_packet *pckt)
{
	ARG_UNUSED(pckt);
	return -ENOTSUP;
}

int espihub_erase_flash(struct espi_flash_packet *pckt)
{
	ARG_UNUSED(pckt);
	return -ENOTSUP;
}
