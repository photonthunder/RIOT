/*
 * Copyright (C) 2017 Travis Griggs <travisgriggs@gmail.com>
 * Copyright (C) 2017 Dan Evans <photonthunder@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     adc_examples
 * @{
 *
 * @file
 * @brief       example application for polling an adc channel
 *
 * @author      Travis Griggs <travisgriggs@gmail.com>
 * @author      Dan Evans <photonthunder@gmail.com>
 *
 * @}
 */

//#include <mini-printf.h>
#include "xtimer.h"
#include "timex.h"
#include "periph/adc.h"
//#include "samd21misc.h"

/* set interval to 1 second */
#define INTERVAL (1U * US_PER_SEC)
volatile uint32_t LoopCount = 0;
int main(void)
{
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

	adc_init(ADC_LINE(0));
	adc_init(ADC_LINE(1));
	adc_init(ADC_LINE(2));

    xtimer_ticks32_t last_wakeup = xtimer_now();
    xtimer_ticks32_t now;
	
    while(1) {
        printf("PA10: %d\n", adc_sample(ADC_LINE(0), ADC_RES_12BIT));
		printf("PA11: %d\n", adc_sample(ADC_LINE(1), ADC_RES_12BIT));
		printf("PA02: %d\n", adc_sample(ADC_LINE(2), ADC_RES_12BIT));
        //printf("BOD33=%08lX\n", SYSCTRL->BOD33.reg);
        now = xtimer_now();
        printf("xtimer=%lu\n",now.ticks32);
//        for (uint32_t index = 0; index < 600000; index++) {
//            LoopCount++;
//        }
//        delayMicroseconds(100000);
        xtimer_periodic_wakeup(&last_wakeup, INTERVAL);
    }

    return 0;
}
