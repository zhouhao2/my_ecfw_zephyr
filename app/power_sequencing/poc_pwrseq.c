/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * PoC power sequencing: eSPI VW monitoring + simulated state steps on EVB.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/espi.h>
#include "pwrplane.h"
#include "espi_hub.h"
#include "board_config.h"
#include "gpio_ec.h"

LOG_MODULE_REGISTER(pwrmgmt, CONFIG_PWRMGT_LOG_LEVEL);

struct pwr_flags g_pwrflags;
static enum system_power_state cur_state = SYSTEM_G3_STATE;
static enum system_power_state next_state = SYSTEM_G3_STATE;
static uint8_t shutdown_reason;
static uint8_t slp_s3 = 1;
static uint8_t slp_s4 = 1;
static uint8_t slp_s5 = 1;

static void pwrseq_slp_handler(uint32_t signal, uint32_t status)
{
	switch (signal) {
	case ESPI_VWIRE_SIGNAL_SLP_S3:
		slp_s3 = status;
		LOG_DBG("VW SLP_S3=%u", status);
		break;
	case ESPI_VWIRE_SIGNAL_SLP_S4:
		slp_s4 = status;
		LOG_DBG("VW SLP_S4=%u", status);
		break;
	case ESPI_VWIRE_SIGNAL_SLP_S5:
		slp_s5 = status;
		LOG_DBG("VW SLP_S5=%u", status);
		break;
	default:
		LOG_DBG("VW signal %u=%u", signal, status);
		break;
	}

	/* VW level: 1 = SLP_Sx# deasserted (high), 0 = asserted (low). */
	if (slp_s5 && slp_s4 && slp_s3) {
		next_state = SYSTEM_S0_STATE;
	} else if (slp_s5 && slp_s4 && !slp_s3) {
		next_state = SYSTEM_S3_STATE;
	} else if (slp_s5 && !slp_s4) {
		next_state = SYSTEM_S4_STATE;
	} else if (!slp_s5) {
		next_state = SYSTEM_S5_STATE;
	}
}

static void pwrseq_sus_handler(uint8_t status)
{
	/* Same contract as pwrplane.c: ACK SUS_WARN from app (driver auto-ACK off). */
	LOG_DBG("VW SUS_WARN=%u -> SUS_ACK", status);
	(void)espihub_send_vw(ESPI_VWIRE_SIGNAL_SUS_ACK, status);
}

static void pwrseq_plt_rst_handler(uint8_t status)
{
	LOG_DBG("VW PLTRST=%u", status);
}

static void espi_bus_reset_handler(uint8_t status)
{
	LOG_DBG("eSPI bus reset sts=%u", status);
}

static const char *state_name(enum system_power_state s)
{
	switch (s) {
	case SYSTEM_G3_STATE: return "G3";
	case SYSTEM_S0_STATE: return "S0";
	case SYSTEM_S3_STATE: return "S3";
	case SYSTEM_S4_STATE: return "S4";
	case SYSTEM_S5_STATE: return "S5";
	default: return "?";
	}
}

void pwrseq_thread(void *p1, void *p2, void *p3)
{
	uint32_t period = p1 ? *(uint32_t *)p1 : 10;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	g_pwrflags.pwr_sw_enabled = 1;
	g_pwrflags.ac_powered = 1;

	(void)espihub_add_state_handler(pwrseq_slp_handler);
	(void)espihub_add_warn_handler(ESPIHUB_SUSPEND_WARNING, pwrseq_sus_handler);
	(void)espihub_add_warn_handler(ESPIHUB_PLATFORM_RESET, pwrseq_plt_rst_handler);
	(void)espihub_add_warn_handler(ESPIHUB_BUS_RESET, espi_bus_reset_handler);

	LOG_DBG("pwrseq PoC started (sim=%d, wait VW)",
		IS_ENABLED(CONFIG_LS_ECFW_POC_SIM_PWRSEQ));

#ifdef CONFIG_LS_ECFW_POC_SIM_PWRSEQ
	k_msleep(500);
	next_state = SYSTEM_S5_STATE;
	k_msleep(200);
	next_state = SYSTEM_S0_STATE;
	LOG_INF("PoC sim advanced to S0 request");
#endif

	while (1) {
		if (cur_state != next_state) {
			LOG_DBG("PWR %s -> %s (SLP S3/S4/S5=%u/%u/%u)",
				state_name(cur_state), state_name(next_state),
				slp_s3, slp_s4, slp_s5);
			cur_state = next_state;
			(void)gpio_write_pin(EC_PWRBTN_LED,
					     cur_state == SYSTEM_S0_STATE);
		}
		k_msleep(period);
	}
}

void pwrseq_error(uint8_t error_code)
{
	LOG_ERR("pwrseq error %u", error_code);
	next_state = SYSTEM_G3_STATE;
}

void set_next_state_to_S5(void)
{
	next_state = SYSTEM_S5_STATE;
}

enum system_power_state pwrseq_system_state(void)
{
	return cur_state;
}

enum boot_config_mode pwrseq_get_boot_mode(void)
{
	return boot_mode_maf ? FLASH_BOOT_MODE_MAF : FLASH_BOOT_MODE_OWN;
}

bool atx_detect(void)
{
	return false;
}

void therm_shutdown(void)
{
	shutdown_reason = SHUTDOWN_REASON_CRTITICAL_THERMAL;
	set_next_state_to_S5();
}

uint8_t read_shutdown_reason(void)
{
	return shutdown_reason;
}

void set_shutdown_reason(uint8_t reason)
{
	shutdown_reason = reason;
}

/* Used by espi_hub wait loops when full pwrseq_utils is not linked */
bool ec_timeout_status(void)
{
	return false;
}

void ec_reset(void)
{
}

void disable_ec_timeout(void)
{
}

void ec_evaluate_timeout(void)
{
}
