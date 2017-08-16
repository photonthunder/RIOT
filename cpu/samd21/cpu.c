/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_samd21
 * @{
 *
 * @file
 * @brief       Implementation of the CPU initialization
 *
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Dan Evans <photonthunder@gmail.com>
 * @}
 */

#include "cpu.h"
#include "periph_conf.h"
#include "periph_clock_config.h"
#include "periph/init.h"

#ifndef VDD
/**
 * @brief   Set system voltage level in mV (determines flash wait states)
 *
 * @note    Override this value in your boards periph_conf.h file
 *          if a different system voltage is used.
 */
#define VDD                 (3300U)
#endif

/* determine the needed flash wait states based on the system voltage (Vdd)
 * see SAMD21 datasheet Rev A (2017) table 37-40 , page 816 */
#if (VDD > 2700)
#define WAITSTATES          ((CLOCK_CORECLOCK - 1) / 24000000)
#else
#define WAITSTATES          ((CLOCK_CORECLOCK - 1) / 14000000)
#endif

/**
 * @brief   Configure clock sources and the cpu frequency
 */
static void clk_init(void)
{
    /* enable clocks for the power, sysctrl and gclk modules */
    PM->APBAMASK.reg = (PM_APBAMASK_PM | PM_APBAMASK_SYSCTRL |
                        PM_APBAMASK_GCLK);

    /* adjust NVM wait states */
    PM->APBBMASK.reg |= PM_APBBMASK_NVMCTRL;
    NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_RWS(WAITSTATES);
    PM->APBBMASK.reg &= ~PM_APBBMASK_NVMCTRL;

#if CLOCK_8MHZ
    /* configure internal 8MHz oscillator to run without prescaler */
    SYSCTRL->OSC8M.bit.PRESC = 0;
    SYSCTRL->OSC8M.bit.ONDEMAND = 1;
    SYSCTRL->OSC8M.bit.RUNSTDBY = 0;
    SYSCTRL->OSC8M.bit.ENABLE = 1;
    while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_OSC8MRDY)) {}
#endif

#if CLOCK_XOSC32K
    /* Use External 32.768KHz Oscillator */
    SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_ONDEMAND |
                           SYSCTRL_XOSC32K_EN32K |
                           SYSCTRL_XOSC32K_XTALEN |
                           SYSCTRL_XOSC32K_STARTUP(6) |
                           SYSCTRL_XOSC32K_RUNSTDBY;
    /* Enable with Seperate Call */
    SYSCTRL->XOSC32K.bit.ENABLE = 1;
#endif

    /* reset the GCLK module so it is in a known state */
    GCLK->CTRL.reg = GCLK_CTRL_SWRST;
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

#if GEN1_1MHZ
#if CLOCK_8MHZ == 0
#error Must turn on CLOCK_8MHZ to use GEN1_1MHZ
#endif
    /* Setup GCLK1 with 1MHz */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(8) |
                         GCLK_GENDIV_ID(1));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_OSC8M |
                         GCLK_GENCTRL_ID(1));
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
#endif

#if GEN2_XOSC32
#if CLOCK_XOSC32K == 0
#error Must turn on CLOCK_XOSC32K to use GEN2_XOSC32
#endif
    /* Setup GCLK2 with XOSC32 */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(1) |
                         GCLK_GENDIV_ID(2));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_XOSC32K |
#if XOSC32_RUNSTDBY
                         GCLK_GENCTRL_RUNSTDBY |
#endif
                         GCLK_GENCTRL_ID(2));
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
#endif

#if GEN3_ULP32K
    /* Setup Clock generator 3 with divider 1 (32.768kHz) */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(1) |
                         GCLK_GENDIV_ID(3));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_OSCULP32K |
                         GCLK_GENCTRL_RUNSTDBY |
                         GCLK_GENCTRL_ID(3));
    
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
#endif

#if CLOCK_USE_PLL
#if GEN1_1MHZ == 0
#error Must turn on GEN1_1MHZ to use CLOCK_USE_PLL
#endif
    /* Set GEN1_1MHZ as source for DFLL */
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_GEN_GCLK1 |
                         GCLK_CLKCTRL_ID_FDPLL |
                         GCLK_CLKCTRL_CLKEN);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    
    /* Enable PLL */
    SYSCTRL->DPLLRATIO.reg = (SYSCTRL_DPLLRATIO_LDR(CLOCK_PLL_MUL));
    SYSCTRL->DPLLCTRLB.reg = (SYSCTRL_DPLLCTRLB_REFCLK_GCLK);
    SYSCTRL->DPLLCTRLA.reg = (SYSCTRL_DPLLCTRLA_ENABLE);
    while(!(SYSCTRL->DPLLSTATUS.reg &
            (SYSCTRL_DPLLSTATUS_CLKRDY |
             SYSCTRL_DPLLSTATUS_LOCK))) {}
    
    /* Select the PLL as source for clock generator 0 (CPU core clock) */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(CLOCK_PLL_DIV) |
                         GCLK_GENDIV_ID(0));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_FDPLL |
                         GCLK_GENCTRL_ID(0));
#elif CLOCK_USE_XOSC32_DFLL
#if CLOCK_XOSC32K == 0
#error Must turn on CLOCK_XOSC32K to use CLOCK_USE_XOSC32_DFLL
#endif
    /* Set GEN2_XOSC32 as source for DFLL */
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_GEN_GCLK2 |
                         GCLK_CLKCTRL_ID_DFLL48 |
                         GCLK_CLKCTRL_CLKEN);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

    /* Disable ONDEMAND mode while writing configurations */
    SYSCTRL->DFLLCTRL.bit.ONDEMAND = 0;
    while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0) {
        /* Wait for DFLL sync */
    }

    /* get the coarse and fine values stored in NVM (Section 9.3) */
    uint32_t coarse = (*(uint32_t *)(0x806024) >> 26);  /* Bits 63:58 */
    uint32_t fine = (*(uint32_t *)(0x806028) & 0x3FF);  /* Bits 73:64 */

    SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(coarse >> 1) |
                           SYSCTRL_DFLLMUL_FSTEP(fine >> 1) |
                           SYSCTRL_DFLLMUL_MUL(CLOCK_CORECLOCK / CLOCK_XOSC32K);
    SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(coarse) |
                           SYSCTRL_DFLLVAL_FINE(fine);
                           SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_MODE;
    while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0) {
        /* Wait for DFLL sync */
    }

    SYSCTRL->DFLLCTRL.bit.ENABLE = 1;
    while ((SYSCTRL->PCLKSR.reg & (SYSCTRL_PCLKSR_DFLLRDY |
                                   SYSCTRL_PCLKSR_DFLLLCKF |
                                   SYSCTRL_PCLKSR_DFLLLCKC)) == 0) {
        /* Wait for DFLLLXXX sync */
    }

    /* select the DFLL as source for clock generator 0 (CPU core clock) */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(1) |
                         GCLK_GENDIV_ID(0));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_DFLL48M |
                         GCLK_GENCTRL_ID(0));
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

    SYSCTRL->DFLLCTRL.bit.ONDEMAND = 1;
    while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0) {
        /* Wait for DFLL sync */
    }
#elif CLOCK_USE_8MHZ_DEFAULT
#if CLOCK_8MHZ == 0
#error Must turn on CLOCK_8MHZ to use CLOCK_USE_8MHZ_DEFAULT
#endif
    /* use internal 8MHz oscillator directly */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(CLOCK_DIV) |
                         GCLK_GENDIV_ID(0));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_OSC8M |
                         GCLK_GENCTRL_ID(0));
#else
#error Must setup GCLK0
#endif
    /* Make sure GCLK0 is on */
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_CLKEN |
                         GCLK_CLKCTRL_GEN_GCLK0);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

    /* redirect all peripherals to a disabled clock generator (7) by default */
    for (int i = 0x3; i <= 0x22; i++) {
        GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_ID(i) |
                             GCLK_CLKCTRL_GEN_GCLK7);
        while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    }
}

void cpu_init(void)
{
    /* disable the watchdog timer */
    WDT->CTRL.bit.ENABLE = 0;
    /* initialize the Cortex-M core */
    cortexm_init();
    /* Initialise clock sources and generic clocks */
    clk_init();
    /* trigger static peripheral initialization */
    periph_init();
}
