/*
 * Copyright (c) 2026 LinkedSemi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Pin policy for EHL_TCP_3L: eSPI, UART3 console, and SIO COM1–COM4
 * keep alternate / GPIO functions. SLP_S3#(PE15) / PLTRST#(PD03) are
 * GPIO inputs. All other pads are detached from FUNC_SEL/PIN_SEL and
 * forced to floating digital input. Motherboard hardware owns power
 * timing; EC does not drive RSMRST.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "gpio_ec.h"
#include "board_config.h"
#include "ls_soc_gpio.h"
#include "reg_sysc_awo.h"
#include "reg_sysc_per.h"
#include "field_manipulate.h"

LOG_MODULE_DECLARE(board, CONFIG_BOARD_LOG_LEVEL);

uint8_t boot_mode_maf;

/* eSPI: PD9 alert + PD10..PD15 data/clk/cs (PIN_SEL0 ESPI_* group). */
#define BOARD_ESPI_PORT		LS_GPIO_PORT_D
#define BOARD_ESPI_PIN_FIRST	9u
#define BOARD_ESPI_PIN_LAST	15u

/* Console UART3 on PH6/PH7. */
#define BOARD_UART3_PORT	LS_GPIO_PORT_H
#define BOARD_UART3_TX_PIN	7u
#define BOARD_UART3_RX_PIN	6u

static bool board_pin_in_range(uint8_t port, uint8_t pin, uint8_t want, uint8_t lo, uint8_t hi)
{
	return port == want && pin >= lo && pin <= hi;
}

static bool board_pin_is_reserved(uint8_t port, uint8_t pin)
{
	if (board_pin_in_range(port, pin, BOARD_ESPI_PORT,
			       BOARD_ESPI_PIN_FIRST, BOARD_ESPI_PIN_LAST)) {
		return true;
	}
	if (port == BOARD_UART3_PORT &&
	    (pin == BOARD_UART3_TX_PIN || pin == BOARD_UART3_RX_PIN)) {
		return true;
	}
	/* COM1 PC05–PC12 */
	if (board_pin_in_range(port, pin, LS_GPIO_PORT_C, 5u, 12u)) {
		return true;
	}
	/* COM2: DCD PE04, CTS/RI/DTR/RTS/DSR/TX/RX PE06–PE12 */
	if (port == LS_GPIO_PORT_E &&
	    (pin == 4u || (pin >= 6u && pin <= 12u))) {
		return true;
	}
	/* COM3 PF02–PF09 */
	if (board_pin_in_range(port, pin, LS_GPIO_PORT_F, 2u, 9u)) {
		return true;
	}
	/* COM4 PG00–PG06 + RX PH00 */
	if (board_pin_in_range(port, pin, LS_GPIO_PORT_G, 0u, 6u) ||
	    (port == LS_GPIO_PORT_H && pin == 0u)) {
		return true;
	}
	return false;
}

/* Leo pinmux index == port*16+pin (same encoding as LSPIN). */
static void board_pinmux_detach(uint8_t sdk_pin)
{
	gpio_port_pin_t *x = (gpio_port_pin_t *)&sdk_pin;
	uint8_t func_io = (uint8_t)(x->port * 16u + x->num);

	MODIFY_REG(SYSC_PER->FUNC_SEL[func_io / 4u],
		   0xffu << (8u * (func_io % 4u)), 0);
	if (func_io >= 96u) {
		SYSC_AWO->PIN_SEL4 &= ~BIT(func_io - 96u);
	} else if (func_io >= 64u) {
		SYSC_AWO->PIN_SEL3 &= ~BIT(func_io - 64u);
	} else if (func_io >= 32u) {
		SYSC_AWO->PIN_SEL2 &= ~BIT(func_io - 32u);
	} else {
		SYSC_AWO->PIN_SEL1 &= ~BIT(func_io);
	}
}

static void board_force_hiz_input(uint8_t sdk_pin)
{
	gpio_port_pin_t *x = (gpio_port_pin_t *)&sdk_pin;
	awo_io_reg_t *io = &SYSC_AWO->IO[x->port];
	uint32_t bit = BIT(x->num);
	uint32_t bit_hi = BIT(x->num + 16);

	board_pinmux_detach(sdk_pin);
	io_cfg_input(sdk_pin);
	io_pull_write(sdk_pin, IO_PULL_DISABLE);
	io->AE &= ~(bit | bit_hi);
}

/*
 * Clear group muxes that can steal pads (keep eSPI / eSPI alert only).
 * Do not touch QSPI if boot ROM needs it for XIP — clear LPC/SPIS/USB/KEY.
 */
static void board_clear_group_pinmux(void)
{
	uint32_t sel0 = SYSC_AWO->PIN_SEL0;

	sel0 &= ~(SYSC_AWO_LPC_EN_MASK | SYSC_AWO_LPC_SERIRQ_EN_MASK |
		  SYSC_AWO_SPIS_EN_MASK | SYSC_AWO_USB_PUPD_MASK |
		  SYSC_AWO_USB_CID_MASK);
	/* Preserve ESPI_EN + ESPI_ALERT_EN */
	SYSC_AWO->PIN_SEL0 = sel0;

	SYSC_AWO->PIN_SEL5 &= ~(SYSC_AWO_KEYI_EN_MASK | SYSC_AWO_KEYO_EN_MASK);
}

static void board_quarantine_unused_pins(void)
{
	uint32_t detached = 0;

	board_clear_group_pinmux();

	for (uint8_t port = 0; port < 8u; port++) {
		for (uint8_t pin = 0; pin < 16u; pin++) {
			uint8_t sdk_pin;

			if (board_pin_is_reserved(port, pin)) {
				continue;
			}
			sdk_pin = (uint8_t)((port << 4) | pin);
			board_force_hiz_input(sdk_pin);
			detached++;
		}
	}

	LOG_INF("pin quarantine: %u pads -> GPIO input (no pull); keep eSPI PD9-15 + UART3 PH6/7 + COM1-4",
		detached);
	LOG_INF("sense PE15(SLP_S3)=%d PD03(PLTRST)=%d PIN_SEL0=0x%08x",
		io_get_input_val(PE15), io_get_input_val(PD03),
		SYSC_AWO->PIN_SEL0);
}

int board_init(void)
{
	boot_mode_maf = 0;

	if (gpio_init()) {
		LOG_ERR("gpio_init failed");
		return -ENODEV;
	}

	/*
	 * Drivers (PWM/SPI/I2C/…) may have applied pinctrl before main.
	 * Board DTS disables those nodes; this sweep clears any residual mux.
	 */
	board_quarantine_unused_pins();

	LOG_INF("ls101x_ehl_tcp_3l board_init done (boot_mode_maf=%u)", boot_mode_maf);
	return 0;
}

int board_suspend(void)
{
	return 0;
}

int board_resume(void)
{
	return 0;
}

#ifdef CONFIG_THERMAL_MANAGEMENT
#include "board_thermal.h"
#include "fan.h"

static struct fan_dev board_fan_tbl[] = {
	{ PWM_CH_UNDEF, TACH_CH_UNDEF },
	{ PWM_CH_UNDEF, TACH_CH_UNDEF },
	{ PWM_CH_UNDEF, TACH_CH_UNDEF },
	{ PWM_CH_UNDEF, TACH_CH_UNDEF },
};

int board_fan_dev_tbl_init(int *size, struct fan_dev **tbl)
{
	*size = ARRAY_SIZE(board_fan_tbl);
	*tbl = board_fan_tbl;
	return 0;
}

int board_therm_sensor_list_init(void)
{
	return 0;
}
#endif
