/*
 * SPDX-FileCopyrightText: 2019 Gunar Schorcht
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup     boards_esp32_heltec-lora32-v3
 * @brief       Board specific definitions for Heltec WiFi LoRa 32 V3 board
 * @{
 *
 * Heltec WiFi LoRa 32 V3 is an ESP32 development board with 8 MB Flash that
 * uses the EPS32 chip directly. It integrates a SemTech SX1262 LoRaWAN
 * communication in the 433 MHz or the 868/915 MHz band, respectively.
 * Additionally, it has an OLED display connected via I2C on board.
 *
 * For detailed information about the configuration of ESP32S3 boards, see
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

/**
 * @name    Button pin definitions
 * @{
 */

/**
 * @brief   Default button GPIO pin definition
 *
 * Generic ESP32 boards have a BOOT button connected to GPIO0, which can be
 * used as button during normal operation. Since the GPIO0 pin is pulled up,
 * the button signal is inverted, i.e., pressing the button will give a
 * low signal.
 */
#define BTN0_PIN  GPIO0

/**
 * @brief   Default button GPIO mode definition
 *
 * Since the GPIO of the button is pulled up with an external resistor, the
 * mode for the GPIO pin has to be GPIO_IN.
 */
#define BTN0_MODE GPIO_IN

/**
 * @brief   Default interrupt flank definition for the button GPIO
 */
#ifndef BTN0_INT_FLANK
#  define BTN0_INT_FLANK GPIO_FALLING
#endif

/**
 * @brief   Definition for compatibility with previous versions
 *
 * May want to try removing to see if necessary
 */
#define BUTTON0_PIN          BTN0_PIN

/** @} */

/**
 * @name    LED (on-board) configuration
 *
 * @{
 */
#define LED0_PIN             GPIO35
#define LED0_ACTIVE          (1) /**< LED is high active */

/** @} */

/**
 * @name        SX126X
 *
 * SX126X configuration.
 * @{
 */
#define SX126X_PARAM_SPI     (SPI_DEV(0))
#define SX126X_PARAM_SPI_NSS GPIO8
#define SX126X_PARAM_RESET   GPIO12
#define SX126X_PARAM_BUSY    GPIO13
#define SX126X_PARAM_DIO0    GPIO14
/** @} */

/* include common board definitions as last step */
#include "board_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* end extern "C" */
#endif

/** @} */
