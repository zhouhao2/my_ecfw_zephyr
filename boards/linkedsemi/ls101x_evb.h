/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * LS101x EVB board signal map for my_ecfw_zephyr PoC.
 * Real RVP power GPIOs are stubs (EC_DUMMY_*); EVB button/LED are live.
 */

#ifndef __LS101X_EVB_BOARD_H__
#define __LS101X_EVB_BOARD_H__

#include "gpio_ec.h"

#define KSC_PLAT_NAME			"LS1010"

extern uint8_t boot_mode_maf;

#define PLATFORM_DATA(x, y)		((x) | ((y) << 8))

#define BOARD_ID_MASK			0x003Fu
#define BOM_ID_MASK			0x01C0u
#define FAB_ID_MASK			0x0600u
#define HW_STRAP_MASK			0xF800u
#define HW_ID_MASK			(FAB_ID_MASK | BOM_ID_MASK | BOARD_ID_MASK)
#define BOARD_ID_OFFSET			0u
#define BOM_ID_OFFSET			6u
#define FAB_ID_OFFSET			9u
#define HW_STRAP_OFFSET			11u

/* GPIO port indices: 0=A ... 7=H, 0xF=dummy */
#define LS_GPIO_PORT_A			0u
#define LS_GPIO_PORT_B			1u
#define LS_GPIO_PORT_C			2u
#define LS_GPIO_PORT_D			3u
#define LS_GPIO_PORT_E			4u
#define LS_GPIO_PORT_F			5u
#define LS_GPIO_PORT_G			6u
#define LS_GPIO_PORT_H			7u

#define EC_DUMMY_GPIO_LOW		EC_GPIO_PORT_PIN(EC_DUMMY_GPIO_PORT, 0x00)
#define EC_DUMMY_GPIO_HIGH		EC_GPIO_PORT_PIN(EC_DUMMY_GPIO_PORT, 0x01)

/* Live EVB pins */
#define EC_GPIO_LED0			EC_GPIO_PORT_PIN(LS_GPIO_PORT_A, 0)
#define EC_GPIO_BTN0			EC_GPIO_PORT_PIN(LS_GPIO_PORT_B, 7)
/* Motherboard sense: SLP_S3# / PLTRST# (input only; see overlay — not PWM8) */
#define EC_GPIO_SLP_S3			EC_GPIO_PORT_PIN(LS_GPIO_PORT_E, 15)
#define EC_GPIO_PLTRST			EC_GPIO_PORT_PIN(LS_GPIO_PORT_D, 3)

/* Device tree aliases used by hubs */
#define ESPI_0				DT_NODELABEL(espi)
#define PECI_0_INST			DT_NODELABEL(peci0)
#define I2C_BUS_0			DT_NODELABEL(i2c1)
#define I2C_BUS_1			DT_NODELABEL(i2c2)
#define EEPROM_DRIVER_I2C_ADDR		0x50
#define IO_EXPANDER_0_I2C_ADDR		0x22

/* Buttons / lid — map power button to EVB SW0; others stub */
#define PWRBTN_EC_IN_N			EC_GPIO_BTN0
#define VOL_UP				EC_DUMMY_GPIO_HIGH
#define VOL_DOWN			EC_DUMMY_GPIO_HIGH
#define HOME_BUTTON			EC_DUMMY_GPIO_HIGH
#define SMC_LID				EC_DUMMY_GPIO_HIGH

#define VOL_UP_INIT_POS			1
#define VOL_DN_INIT_POS			1
#define PWR_BTN_INIT_POS		1
#define HOME_INIT_POS			1
#define LID_INIT_POS			1

/* Power / eSPI related — stubs for EVB PoC */
#define PM_PWRBTN			EC_DUMMY_GPIO_HIGH
#define PM_RSMRST			EC_DUMMY_GPIO_HIGH
#define RSMRST_PWRGD			EC_DUMMY_GPIO_HIGH
#define PCH_PWROK			EC_DUMMY_GPIO_HIGH
#define ALL_SYS_PWRGD			EC_DUMMY_GPIO_HIGH
#define BC_ACOK				EC_DUMMY_GPIO_HIGH
#define ESPI_RESET_MAF			EC_DUMMY_GPIO_HIGH
#define ESPI_RESET_G3SAF		EC_DUMMY_GPIO_HIGH
#define WAKE_SCI			EC_DUMMY_GPIO_HIGH
#define EC_PWRBTN_LED			EC_GPIO_LED0
#define SLP_S0_PLT_EC_N			EC_DUMMY_GPIO_HIGH
#define CPU_C10_GATE			EC_DUMMY_GPIO_HIGH

#define FAN_PWR_DISABLE_N		EC_DUMMY_GPIO_HIGH

#define VIRTUAL_BAT			EC_DUMMY_GPIO_HIGH
#define VIRTUAL_DOCK			EC_DUMMY_GPIO_HIGH
#define THERM_STRAP			EC_DUMMY_GPIO_HIGH
#define VIRTUAL_BAT_INIT_POS		1
#define VIRTUAL_DOCK_INIT_POS		1
#define G3_SAF_DETECT			EC_DUMMY_GPIO_LOW

#endif /* __LS101X_EVB_BOARD_H__ */
