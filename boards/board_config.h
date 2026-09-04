/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BOARD_COMMON_H__
#define __BOARD_COMMON_H__

/**
 * @brief This global variable helps to configure variable gpios.
 */
extern uint8_t boot_mode_maf;

#if defined(CONFIG_BOARD_LS101X_EHL_TCP_3L)
#include "ls101x_ehl_tcp_3l.h"
#elif defined(CONFIG_BOARD_LS101X_EVB) || defined(CONFIG_SOC_LS1010)
#include "ls101x_evb.h"
#else
#error "Platform not supported — build for ls101x_ehl_tcp_3l or ls101x_evb"
#endif

#ifdef CONFIG_THERMAL_MANAGEMENT
#include "thermalmgmt.h"
#include "board_thermal.h"
#endif

/**
 * @brief Perform platform configuration depending on the board.
 */
int board_init(void);

/**
 * @brief Perform platform configuration during suspend depending on the board.
 */
int board_suspend(void);

/**
 * @brief Perform platform configuration during resume depending on the board.
 */
int board_resume(void);

#endif /* __BOARD_COMMON_H__ */
