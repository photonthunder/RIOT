/* Here is the UART code */
    /* set asynchronous mode w/o parity, LSB first, TX and RX pad as specified
     * by the board in the periph_conf.h, x16 sampling and use internal clock */
    dev(uart)->CTRLA.reg = (SERCOM_USART_CTRLA_DORD |
                      SERCOM_USART_CTRLA_SAMPR(0x1) |
                      SERCOM_USART_CTRLA_TXPO(uart_config[uart].tx_pad) |
                      SERCOM_USART_CTRLA_RXPO(uart_config[uart].rx_pad) |
                            SERCOM_USART_CTRLA_RUNSTDBY |
                      SERCOM_USART_CTRLA_MODE(0x1));
    /* Set run in standby mode if enabled */
    if (uart_config[uart].flags & UART_FLAG_RUN_STANDBY) {
        dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_RUNSTDBY;
    }

    /* calculate and set baudrate */
    uint32_t baud = ((((uint32_t)CLOCK_CORECLOCK * 10) / baudrate) / 16);
    dev(uart)->BAUD.FRAC.FP = (baud % 10);
    dev(uart)->BAUD.FRAC.BAUD = (baud / 10);

    /* enable transmitter, and configure 8N1 mode */
    dev(uart)->CTRLB.reg = (SERCOM_USART_CTRLB_TXEN);
    /* enable receiver and RX interrupt if configured */
    if (rx_cb) {
        uart_ctx[uart].rx_cb = rx_cb;
        uart_ctx[uart].arg = arg;
        NVIC_EnableIRQ(SERCOM0_IRQn + sercom_id(dev(uart)));
        dev(uart)->CTRLB.reg |= SERCOM_USART_CTRLB_RXEN;
        dev(uart)->INTENSET.reg |= SERCOM_USART_INTENSET_RXC;
        dev(uart)->INTENSET.reg |= SERCOM_USART_INTENSET_RXS;
        /* set wakeup receive from sleep if enabled */
        if (uart_config[uart].flags & UART_FLAG_WAKEUP) {
            dev(uart)->CTRLB.reg |= SERCOM_USART_CTRLB_SFDE;
        }
    }
    while (dev(uart)->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB) {}

    /* and finally enable the device */
    dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

volatile bool Uart1Active;
static inline void irq_handler(unsigned uartnum)
{
    if (dev(uartnum)->INTFLAG.reg & SERCOM_USART_INTFLAG_RXC) {
        /* interrupt flag is cleared by reading the data register */
        //Uart1Active = true;
        uart_ctx[uartnum].rx_cb(uart_ctx[uartnum].arg,
                                (uint8_t)(dev(uartnum)->DATA.reg));
    }
    else if (dev(uartnum)->INTFLAG.reg & SERCOM_USART_INTFLAG_RXS) {
        Uart1Active = true;
        dev(uartnum)->INTFLAG.reg = SERCOM_USART_INTFLAG_RXS;
    }
    else if (dev(uartnum)->INTFLAG.reg & SERCOM_USART_INTFLAG_ERROR) {
        /* clear error flag */
        dev(uartnum)->INTFLAG.reg = SERCOM_USART_INTFLAG_ERROR;
    }
    
    cortexm_isr_end();
}

/* Here is the DFLL and PLL code */

/* setup 1 MHz (8MHz clock/8) clock on Generic Clock Generator 1 */
static bool is_enabled_gen1_1MHz = false;
void setup_gen1_1MHz(void) {
    if (is_enabled_gen1_1MHz) {
        return;
    }
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    /* Setup GCLK1 with 1MHz */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(8) |
                         GCLK_GENDIV_ID(1));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_OSC8M |
                         GCLK_GENCTRL_ID(1));
    GCLK->GENCTRL.bit.RUNSTDBY = 1;
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    is_enabled_gen1_1MHz = true;
    return;
}

/* setup 32 kHz XOSC on Generic Clock Generator 2 */
static bool gen2_enabled = false;
bool is_enabled_gen2_xosc32(void) {
    return gen2_enabled;
}

static bool run_standby_gen2_xosc32 = false;
void setup_gen2_xosc32(bool standby) {
    if ((standby == true) && (run_standby_gen2_xosc32 == false)) {
        GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2);
        GCLK->GENCTRL.bit.GENEN = 0;
        while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
        gen2_enabled = false;
        run_standby_gen2_xosc32 = true;
    }
    if (gen2_enabled) {
        return;
    }
    
    /* Use External 32.768KHz Oscillator */
    SYSCTRL->XOSC32K.reg = //SYSCTRL_XOSC32K_ONDEMAND |
    SYSCTRL_XOSC32K_EN32K |
    SYSCTRL_XOSC32K_XTALEN |
    SYSCTRL_XOSC32K_STARTUP(6) |
    SYSCTRL_XOSC32K_RUNSTDBY;
    /* Enable with Seperate Call */
    SYSCTRL->XOSC32K.bit.ENABLE = 1;
    
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    /* Setup GCLK2 with XOSC32 */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(1) |
                         GCLK_GENDIV_ID(2));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_SRC_XOSC32K |
                         GCLK_GENCTRL_ID(2));
    //if (run_standby_gen2_xosc32) {
    GCLK->GENCTRL.bit.RUNSTDBY = 1;
    //}
    GCLK->GENCTRL.bit.GENEN = 1;
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    gen2_enabled = true;
}

/* setup 32 kHz ULP internal clock on Generic Clock Generator 3 */
static bool is_enabled_gen3_ULP32k = false;
void setup_gen3_ULP32k(void) {
    if (is_enabled_gen3_ULP32k) {
        return;
    }
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    /* Setup Clock generator 3 with divider 32 (32.768kHz/32 = 1.024 kHz) */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(4) |
                         GCLK_GENDIV_ID(3));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_OSCULP32K |
                         GCLK_GENCTRL_RUNSTDBY |
                         GCLK_GENCTRL_DIVSEL |
                         GCLK_GENCTRL_ID(3));
    
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    is_enabled_gen3_ULP32k = true;
}

/* configure clock sources and the cpu frequency */
static void clk_init(void)
{
    /* enable clocks for the power, sysctrl and gclk modules */
    PM->APBAMASK.reg = (PM_APBAMASK_PM | PM_APBAMASK_SYSCTRL |
                        PM_APBAMASK_GCLK);
    
    /* Errata 13140 */
    NVMCTRL->CTRLB.bit.SLEEPPRM = 3;
    NVMCTRL->CTRLB.bit.MANW = 1;
    /* adjust NVM wait states */
    PM->APBBMASK.reg |= PM_APBBMASK_NVMCTRL;
    NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_RWS(WAITSTATES);
    PM->APBBMASK.reg &= ~PM_APBBMASK_NVMCTRL;
    
    /* configure internal 8MHz oscillator to run without prescaler */
    SYSCTRL->OSC8M.bit.PRESC = 0;
    //SYSCTRL->OSC8M.bit.ONDEMAND = 1;
    SYSCTRL->OSC8M.bit.RUNSTDBY = 1;
    SYSCTRL->OSC8M.bit.ENABLE = 1;
    while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_OSC8MRDY)) {}
    
    /* reset the GCLK module so it is in a known state */
    GCLK->CTRL.reg = GCLK_CTRL_SWRST;
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    
#if CLOCK_SRC == CLOCK_USE_PLL
    setup_gen1_1MHz();
    /* Set GEN1_1MHZ as source for PLL */
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_GEN_GCLK1 |
                         GCLK_CLKCTRL_ID_FDPLL |
                         GCLK_CLKCTRL_CLKEN);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    
    /* Enable PLL */
    SYSCTRL->DPLLRATIO.reg = (SYSCTRL_DPLLRATIO_LDR(CLOCK_PLL_MUL));
    SYSCTRL->DPLLCTRLB.reg = (SYSCTRL_DPLLCTRLB_REFCLK_GCLK);
    SYSCTRL->DPLLCTRLA.reg = (SYSCTRL_DPLLCTRLA_RUNSTDBY | SYSCTRL_DPLLCTRLA_ENABLE);
    while(!(SYSCTRL->DPLLSTATUS.reg &
            (SYSCTRL_DPLLSTATUS_CLKRDY |
             SYSCTRL_DPLLSTATUS_LOCK))) {}
    
    /* Select the PLL as source for clock generator 0 (CPU core clock) */
    GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(CLOCK_DIV) |
                         GCLK_GENDIV_ID(0));
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN |
                         GCLK_GENCTRL_SRC_FDPLL |
                         GCLK_GENCTRL_RUNSTDBY |
                         GCLK_GENCTRL_ID(0));
#elif CLOCK_SRC == CLOCK_USE_DFLL
    setup_gen2_xosc32(true);
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
    SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_RUNSTDBY;
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
                         GCLK_GENCTRL_RUNSTDBY |
                         GCLK_GENCTRL_ID(0));
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    
    //SYSCTRL->DFLLCTRL.bit.ONDEMAND = 1;
    while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0) {
        /* Wait for DFLL sync */
    }
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



