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
 * @brief           CPU specific definitions for default clock configuration
 *
 * @author          Daniel Evans <photonthunder@gmail.com>
 */

#ifndef PERIPH_CLOCK_CONFIG_H
#define PERIPH_CLOCK_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Default Clock configurations
 */
    
/* If RTC or RTT is on Need XOSC32 */
#if RTC_NUMOF || RTT_NUMOF
#ifndef CLOCK_XOSC32K
#define CLOCK_XOSC32K       (32768UL)
#endif
#ifndef GEN2_XOSC32
#define GEN2_XOSC32         (1)
#endif
#endif
    
/* If never set then use these defaults */
#ifndef CLOCK_8MHZ
#define CLOCK_8MHZ              (8000000UL)
#endif

#ifndef CLOCK_XOSC32K
#define CLOCK_XOSC32K           (0)
#endif
    
#ifndef XOSC32_RUNSTDBY
#define XOSC32_RUNSTDBY         (0)
#endif

#ifndef GEN1_1MHZ
#define GEN1_1MHZ               (1)
#endif

#ifndef GEN2_XOSC32
#define GEN2_XOSC32             (0)
#endif

#ifndef GEN3_ULP32K
#define GEN3_ULP32K             (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CLOCK_CONFIG_H */
/** @} */
