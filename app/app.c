/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/logging/log.h>
#include "board.h"
#include "board_config.h"
#include "espi_hub.h"
#include "pwrplane.h"
#include "task_handler.h"
#include "softstrap.h"

LOG_MODULE_REGISTER(ecfw, CONFIG_EC_LOG_LEVEL);

int main(void)
{
	int ret;

	k_sleep(K_SECONDS(CONFIG_EC_DELAYED_BOOT));
	k_msleep(100);

	LOG_INF("LS EC FW (Leo) %p board=%s", k_current_get(), CONFIG_BOARD);

	ret = board_devices_check();
	if (ret) {
		LOG_ERR("Device drivers check fail %d", ret);
		return ret;
	}

	ret = espihub_init();
	if (ret) {
		LOG_ERR("Failed to init espi %d", ret);
		return ret;
	}

	ret = board_init();
	if (ret) {
		LOG_ERR("Failed to init board %d", ret);
		return ret;
	}

	(void)read_board_id();
	strap_init();
	start_all_tasks();

	LOG_INF("All EC tasks started");

	while (true) {
		k_msleep(2100);
	}
}
