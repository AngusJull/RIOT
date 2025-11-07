/*
 * SPDX-FileCopyrightText: 2019 Gunar Schorcht
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup     boards_esp32_heltec-lora32-v3
 * @brief       Peripheral MCU configuration for Heltec WiFi LoRa 32 V3 board
 * @{
 *
 * Heltec WiFi LoRa 32 V3 is an ESP32 development board with 8 MB Flash that
 * uses the EPS32 chip directly. It integrates a SemTech SX1276 or SX1278 for
 * LoRaWAN communication in the 433 MHz or the 868/915 MHz band, respectively.
 * Additionally, it has an OLED display connected via I2C on board.
 *
 * For detailed information about the configuration of ESP32 boards, see
 * section \ref esp32_peripherals "Common Peripherals".
 *
 * @note
 * Most definitions can be overridden by an \ref esp32_application_specific_configurations
 * "application-specific board configuration".
 *
 * @file
 * @author      Gunar Schorcht <gunar@schorcht.net>
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    ADC and DAC channel configuration
 * @{
 */

/**
 * @brief   Declaration of GPIOs that can be used as ADC channels
 *
 * @note As long as the GPIOs listed in ADC_GPIOS are not initialized as ADC
 * channels with the \ref adc_init function, they can be used for other
 * purposes.
 */
#ifndef ADC_GPIOS
#  define ADC_GPIOS { GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7 GPIO19, GPIO20 }
#endif

/**
 * @brief   Declaration of GPIOs that can be used as DAC channels
 *
 * @note As long as the GPIOs listed in DAC_GPIOS are not initialized as DAC
 * channels with the \ref dac_init function, they can be used for other
 * purposes.
 *
 * Since all DAC GPIOs are used for board control functions, there are no
 * GPIOs left that can be used as DAC channels.
 */
#ifndef DAC_GPIOS
#  define DAC_GPIOS \
      {             \
      }
#endif
/** @} */

/**
 * @name   I2C configuration
 *
 * Only I2C interface I2C_DEV(0) is used.
 *
 * @note The GPIOs listed in the configuration are only initialized as I2C
 * signals when module `periph_i2c` is used. Otherwise they are not
 * allocated and can be used for other purposes.
 *
 * @{
 */
#ifndef I2C0_SPEED
#  define I2C0_SPEED I2C_SPEED_FAST /**< I2C bus speed of I2C_DEV(0) */
#endif
#ifndef I2C0_SCL
#  define I2C0_SCL GPIO18 /**< SCL signal of I2C_DEV(0) [UEXT1] */
#endif
#ifndef I2C0_SDA
#  define I2C0_SDA GPIO17 /**< SDA signal of I2C_DEV(0) [UEXT1] */
#endif
/** @} */

/**
 * @name   PWM channel configuration
 *
 * @note As long as the according PWM device is not initialized with
 * the `pwm_init`, the GPIOs declared for this device can be used
 * for other purposes.
 *
 * The ESP32S3 allows for two motor control PWM pins
 *
 * @{
 */
/** PWM channels for device PWM_DEV(0) */
#ifndef PWM0_GPIOS
#  define PWM0_GPIOS { GPIO48, GPIO47 }
#endif
/** @} */

/**
 * @name    SPI configuration
 *
 * @note The GPIOs listed in the configuration are first initialized as SPI
 * signals when the corresponding SPI interface is used for the first time
 * by either calling the `spi_init_cs` function or the `spi_acquire`
 * function. That is, they are not allocated as SPI signals before and can
 * be used for other purposes as long as the SPI interface is not used.
 *
 * @{
 */
#ifndef SPI0_CTRL
#  define SPI0_CTRL FSPI /**< FSPI is used as SPI_DEV(0) */
#endif
#ifndef SPI0_SCK
#  define SPI0_SCK GPIO9 /**< Non-Default FSPI SCK */
#endif
#ifndef SPI0_MISO
#  define SPI0_MISO GPIO11 /**< Non-default FSPI MISO */
#endif
#ifndef SPI0_MOSI
#  define SPI0_MOSI GPIO10 /**< Non-default FSPI MOSI */
#endif
#ifndef SPI0_CS0
#  define SPI0_CS0 GPIO8 /**< Non-default FSPI CS */
#endif
/** @} */

/**
 * @name   UART configuration
 *
 * ESP32-S3 provides 3 UART interfaces at maximum:
 *
 * UART_DEV(0) uses fixed standard configuration.<br>
 * UART_DEV(1) is not used.<br>
 *
 * @{
 */
#define UART0_TXD GPIO43 /**< direct I/O pin for UART_DEV(0) TxD, can't be changed */
#define UART0_RXD GPIO44 /**< direct I/O pin for UART_DEV(0) RxD, can't be changed */

#ifdef __cplusplus
} /* end extern "C" */
#endif

/* include common board definitions as last step */
#include "periph_conf_common.h"

/** @} */
