/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal SMC/ACPI host command path for Leo PoC.
 * ACPI EC 0x60/0x64 is served by ls-host-kcs + ls-kcs (see overlay).
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include "smchost.h"
#include "acpi.h"
#include "acpi_region.h"
#include "sci.h"
#include "pwrplane.h"
#include "pwrbtnmgmt.h"
#include "espi_hub.h"

#if defined(CONFIG_KCS) && DT_NODE_HAS_STATUS(DT_NODELABEL(e8042), okay)
#include <zephyr/drivers/kcs.h>
#define SMCHOST_HAS_HW_KCS 1
#else
#define SMCHOST_HAS_HW_KCS 0
#endif

LOG_MODULE_REGISTER(smchost, CONFIG_SMCHOST_LOG_LEVEL);

struct acpi_tbl g_acpi_tbl;
struct acpi_state_flags g_acpi_state_flags;

uint8_t host_req[SMCHOST_MAX_BUF_SIZE];
uint8_t host_res[SMCHOST_MAX_BUF_SIZE];
uint8_t host_req_len;
uint8_t host_res_len;
uint8_t host_res_idx;
uint8_t peci_access_mode;

#ifdef CONFIG_SMCHOST_EVENT_DRIVEN_TASK
static struct k_sem smc_evt;

void smchost_signal_request(void)
{
	k_sem_give(&smc_evt);
}
#endif

void send_to_host(uint8_t *pdata, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) {
		if (acpi_send_byte(ACPI_EC_0, pdata[i])) {
			LOG_ERR("acpi_send_byte timeout");
			break;
		}
	}
}

static uint8_t smchost_req_length(uint8_t command)
{
	switch (command) {
	case EC_READ:
		return 1;
	case EC_WRITE:
		return 2;
	default:
		return 0;
	}
}

static void generate_sci_pulse(void)
{
	if (!g_acpi_state_flags.sci_enabled || !g_acpi_state_flags.acpi_mode) {
		return;
	}
	if (pwrseq_system_state() != SYSTEM_S0_STATE) {
		return;
	}
#ifdef CONFIG_SMCHOST_SCI_OVER_ESPI
	(void)espihub_send_vw(ESPI_VWIRE_SIGNAL_SCI, ESPIHUB_VW_LOW);
	k_busy_wait(100);
	(void)espihub_send_vw(ESPI_VWIRE_SIGNAL_SCI, ESPIHUB_VW_HIGH);
#endif
}

static void handle_ec_command(uint8_t cmd)
{
	uint8_t rsp = 0;
	uint8_t *ecram = (uint8_t *)&g_acpi_tbl;

	switch (cmd) {
	case EC_QUERY:
		send_to_host(&rsp, 1);
		LOG_INF("EC_QUERY -> 0x%02x", rsp);
		break;
	case EC_READ:
		if (host_req[1] < sizeof(g_acpi_tbl)) {
			rsp = ecram[host_req[1]];
		}
		send_to_host(&rsp, 1);
		LOG_INF("EC_READ idx=0x%02x -> 0x%02x", host_req[1], rsp);
		break;
	case EC_WRITE:
		if (host_req[1] < sizeof(g_acpi_tbl)) {
			ecram[host_req[1]] = host_req[2];
		}
		LOG_INF("EC_WRITE idx=0x%02x val=0x%02x", host_req[1], host_req[2]);
		break;
	case EC_BURST:
	case EC_NORM:
		LOG_DBG("ACPI burst/normal 0x%02x", cmd);
		break;
	default:
		LOG_INF("SMC cmd 0x%02x (unhandled PoC)", cmd);
		break;
	}
}

static void poll_acpi_host(void)
{
	while (acpi_get_flag(ACPI_EC_0, ACPI_FLAG_IBF)) {
		if (acpi_get_flag(ACPI_EC_0, ACPI_FLAG_CD)) {
			host_req_len = 0;
			host_req[0] = acpi_read_idr(ACPI_EC_0);
			LOG_DBG("EC cmd 0x%02x", host_req[0]);
		} else if (host_req_len < SMCHOST_MAX_BUF_SIZE) {
			host_req[host_req_len] = acpi_read_idr(ACPI_EC_0);
			LOG_DBG("EC data[%u]=0x%02x", host_req_len, host_req[host_req_len]);
		} else {
			(void)acpi_read_idr(ACPI_EC_0);
			LOG_WRN("ACPI host_req overflow");
			return;
		}

		if (host_req[0]) {
			if ((host_req[0] == EC_READ) || (host_req[0] == EC_WRITE)) {
				generate_sci_pulse();
			}
			if (smchost_req_length(host_req[0]) == host_req_len) {
				handle_ec_command(host_req[0]);
				host_req[0] = 0;
			}
		}
		host_req_len++;
	}
}

static void smchost_pwrbtn_evt(uint8_t level)
{
	LOG_INF("SMC pwrbtn level=%u pwr_state=%d", level, pwrseq_system_state());
	g_acpi_tbl.acpi_flags2.pwr_btn = level ? 1 : 0;
}

#if SMCHOST_HAS_HW_KCS && defined(CONFIG_SMCHOST_EVENT_DRIVEN_TASK)
static void kcs_ibf_cb(const struct device *dev, void *param)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(param);
	smchost_signal_request();
}
#endif

void smchost_thread(void *p1, void *p2, void *p3)
{
	uint32_t period = p1 ? *(uint32_t *)p1 : 10;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

#ifdef CONFIG_SMCHOST_EVENT_DRIVEN_TASK
	k_sem_init(&smc_evt, 0, 1);
#endif

	memset(&g_acpi_tbl, 0, sizeof(g_acpi_tbl));
	g_acpi_tbl.acpi_space = 0x01;
	g_acpi_state_flags.acpi_mode = 1;
	g_acpi_state_flags.sci_enabled = 1;

	pwrbtn_register_handler(smchost_pwrbtn_evt);

#if SMCHOST_HAS_HW_KCS && defined(CONFIG_SMCHOST_EVENT_DRIVEN_TASK)
	{
		const struct device *kcs = DEVICE_DT_GET(DT_NODELABEL(e8042));

		if (device_is_ready(kcs)) {
			(void)kcs_set_ibf_callback(kcs, kcs_ibf_cb, NULL);
		}
	}
#endif

	LOG_INF("smchost PoC started");

	while (1) {
#ifdef CONFIG_SMCHOST_EVENT_DRIVEN_TASK
		(void)k_sem_take(&smc_evt, K_MSEC(period));
		poll_acpi_host();
#else
		poll_acpi_host();
		k_msleep(period);
#endif
	}
}

uint8_t get_pln_pin_sts(void)
{
	return PLN_PIN_NC;
}

void set_pln_pin_sts(uint8_t sts)
{
	ARG_UNUSED(sts);
}

uint8_t get_pltrst_signal_sts(void)
{
	return 1;
}

void manage_pln_signal(void)
{
}

uint8_t check_btn_sci_sts(uint8_t btn_sci_en_dis)
{
	ARG_UNUSED(btn_sci_en_dis);
	return 0;
}

/* SCI stubs (full sci.c not linked in PoC) */
void sci_queue_init(void)
{
}

void sci_queue_flush(void)
{
}

void generate_sci(void)
{
	generate_sci_pulse();
}

void check_sci_queue(void)
{
}

bool sci_pending(void)
{
	return false;
}

void send_sci_events(void)
{
}

void enqueue_sci(uint8_t Code)
{
	ARG_UNUSED(Code);
}

bool is_system_in_acpi_mode(void)
{
	return g_acpi_state_flags.acpi_mode;
}
