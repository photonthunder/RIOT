/*
 * Copyright (C) 2017 Daniel Evans <photonthunder@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup         cpu_samd21
 * @{
 *
 * @file
 * @brief           CPU specific definitions for generic clock generators
 *
 * @author          Daniel Evans <photonthunder@gmail.com>
 */

#ifndef PERIPH_CLOCK_CONFIG_H
#define PERIPH_CLOCK_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name generic clock generator 1
 * @brief setup 1 MHz clock on generic clock generator 1
 * @{
 */
uint8_t setup_gen1_1MHz(void);
/** @} */

/**
 * @name generic clock generator 2
 * @brief setup 32 kHz XOSC on generic clock generator 2
 * @{
 */
void setup_gen2_xosc32(bool standby);
/** @} */

/**
 * @name check generic clock generator 2
 * @brief check if GCLK2 is enabled
 * @{
 */
bool is_gen2_xosc32_enabled(void);
/** @} */
    
/**
 * @name generic clock generator 3
 * @brief setup 32 kHz ULP internal clock on generic clock generator 3
 * @{
 */
void setup_gen3_ULP32k(void);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CLOCK_CONFIG_H */
/** @} */
