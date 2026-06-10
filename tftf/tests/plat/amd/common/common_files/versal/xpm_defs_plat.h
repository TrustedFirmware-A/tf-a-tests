/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Versal platform-specific PM definitions.
 */

#ifndef XPM_DEFS_PLAT_H_
#define XPM_DEFS_PLAT_H_

/* Versal pin function IDs as defined in xilpm/xpm_defs.h (auto-numbered) */
enum pm_pin_fun_ids {
	PIN_FUNC_SPI0,			/**< Pin function ID of SPI0 */
	PIN_FUNC_SPI0_SS,		/**< Pin function ID of SPI0_SS */
	PIN_FUNC_SPI1,			/**< Pin function ID of SPI1 */
	PIN_FUNC_SPI1_SS,		/**< Pin function ID of SPI1_SS */
	PIN_FUNC_CAN0,			/**< Pin function ID of CAN0 */
	PIN_FUNC_CAN1,			/**< Pin function ID of CAN1 */
	PIN_FUNC_I2C0,			/**< Pin function ID of I2C0 */
	PIN_FUNC_I2C1,			/**< Pin function ID of I2C1 */
	PIN_FUNC_I2C_PMC,		/**< Pin function ID of I2C_PMC */
	PIN_FUNC_TTC0_CLK,		/**< Pin function ID of TTC0_CLK */
	PIN_FUNC_TTC0_WAV,		/**< Pin function ID of TTC0_WAV */
	PIN_FUNC_TTC1_CLK,		/**< Pin function ID of TTC1_CLK */
	PIN_FUNC_TTC1_WAV,		/**< Pin function ID of TTC1_WAV */
	PIN_FUNC_TTC2_CLK,		/**< Pin function ID of TTC2_CLK */
	PIN_FUNC_TTC2_WAV,		/**< Pin function ID of TTC2_WAV */
	PIN_FUNC_TTC3_CLK,		/**< Pin function ID of TTC3_CLK */
	PIN_FUNC_TTC3_WAV,		/**< Pin function ID of TTC3_WAV */
	PIN_FUNC_WWDT0,			/**< Pin function ID of WWDT0 */
	PIN_FUNC_WWDT1,			/**< Pin function ID of WWDT1 */
	PIN_FUNC_SYSMON_I2C0,		/**< Pin function ID of SYSMON_I2C0 */
	PIN_FUNC_SYSMON_I2C0_ALERT,	/**< Pin function ID of SYSMON_I2C0_ALERT */
	PIN_FUNC_UART0,			/**< Pin function ID of UART0 */
	PIN_FUNC_UART0_CTRL,		/**< Pin function ID of UART0_CTRL */
	PIN_FUNC_UART1,			/**< Pin function ID of UART1 */
	PIN_FUNC_UART1_CTRL,		/**< Pin function ID of UART1_CTRL */
	PIN_FUNC_GPIO0,			/**< Pin function ID of GPIO0 */
	PIN_FUNC_GPIO1,			/**< Pin function ID of GPIO1 */
	PIN_FUNC_GPIO2,			/**< Pin function ID of GPIO2 */
	PIN_FUNC_EMIO0,			/**< Pin function ID of EMIO0 */
	PIN_FUNC_GEM0,			/**< Pin function ID of GEM0 */
	PIN_FUNC_GEM1,			/**< Pin function ID of GEM1 */
	PIN_FUNC_TRACE0,		/**< Pin function ID of TRACE0 */
	PIN_FUNC_TRACE0_CLK,		/**< Pin function ID of TRACE0_CLK */
	PIN_FUNC_MDIO0,			/**< Pin function ID of MDIO0 */
	PIN_FUNC_MDIO1,			/**< Pin function ID of MDIO1 */
	PIN_FUNC_GEM_TSU0,		/**< Pin function ID of GEM_TSU0 */
	PIN_FUNC_PCIE0,			/**< Pin function ID of PCIE0 */
	PIN_FUNC_SMAP0,			/**< Pin function ID of SMAP0 */
	PIN_FUNC_USB0,			/**< Pin function ID of USB0 */
	PIN_FUNC_SD0,			/**< Pin function ID of SD0 */
	PIN_FUNC_SD0_PC,		/**< Pin function ID of SD0_PC */
	PIN_FUNC_SD0_CD,		/**< Pin function ID of SD0_CD */
	PIN_FUNC_SD0_WP,		/**< Pin function ID of SD0_WP */
	PIN_FUNC_SD1,			/**< Pin function ID of SD1 */
	PIN_FUNC_SD1_PC,		/**< Pin function ID of SD1_PC */
	PIN_FUNC_SD1_CD,		/**< Pin function ID of SD1_CD */
	PIN_FUNC_SD1_WP,		/**< Pin function ID of SD1_WP */
	PIN_FUNC_OSPI0,			/**< Pin function ID of OSPI0 */
	PIN_FUNC_OSPI0_SS,		/**< Pin function ID of OSPI0_SS */
	PIN_FUNC_QSPI0,			/**< Pin function ID of QSPI0 */
	PIN_FUNC_QSPI0_FBCLK,		/**< Pin function ID of QSPI0_FBCLK */
	PIN_FUNC_QSPI0_SS,		/**< Pin function ID of QSPI0_SS */
	PIN_FUNC_TEST_CLK,		/**< Pin function ID of TEST_CLK */
	PIN_FUNC_TEST_SCAN,		/**< Pin function ID of TEST_SCAN */
	PIN_FUNC_TAMPER_TRIGGER,	/**< Pin function ID of TAMPER_TRIGGER */
	MAX_FUNCTION,			/**< Max Pin function */
};

#endif /* XPM_DEFS_PLAT_H_ */
