
#include "k66.h"

#include <stdint.h>

void go()
{
    asm volatile ("cpsid i\n" ::: "memory");

    WDOG->UNLOCK = 0xc520;
    WDOG->UNLOCK = 0xd928;
    __asm__ volatile ("nop");
    __asm__ volatile ("nop");
    WDOG->STCTRLH = 0x10;

    // Enable GPIOs on port C...
    SIM->CGC5 |= 0x800;

    // Default PTA18/19 is XTAL.
    // Enable port A also, although that may be unnecessary?
    // SIM_SCGC5 |= 0x200;

    // Set RANGE=v. high frequency, HGO=low gain, EREFS=oscillator ...
    MCG->C2 = 0x24;
    // Set the OSC to enabled, run in stop, 18pf load.
    OSC->CR = 0xa9;

    // Wait for the OSCINIT0 bit to come through...
    while (~MCG->S & 2);

    // Switch the MCGOUTCLK to CLKS=2, erefclk.
    MCG->C1 = (MCG->C1 & ~0xc0) | 0x80;

    // Wait for the switch...
    while ((MCG->S & 0xc) != 8);

    // Switch the FLL clock to the eclk, /512.
    MCG->C1 = 0xa0;
    // Wait for IREF to switch.
    while (MCG->S & 0x10);

    // Now configure the PLL for 144 Mhz = 8 x 18.  So /2 and *18.
    MCG->C6 = 2;                        // *18.
    MCG->C5 = 0x61;                     // Enable, Stop-en, /2

    // Wait for lock...
    _Static_assert(&MCG->S == (void *) 0x40064006, "MCG S");
    while (~MCG->S & 0x40);

    // Now select PLLS from PLL not FLL.  Keep *18.
    _Static_assert(&MCG->C6 == (void *) 0x40064005, "MCG C6");
    MCG->C6 = 0x42;

    // Wait for PLLS to take effect...
    while (~MCG->S & 0x20);

    // The flash clock has a max. of 28MHz.  Set it to /6 (will give 24MHz).
    SIM->CLKDIV1 = (SIM->CLKDIV1 & ~0xf0000) | 0x50000;

    // Now switch CLKS to the PLL...
    MCG->C1 = MCG->C1 & ~0xc0;

    // And wait for it to take effect.
    while ((MCG->S & 0xc) != 0xc);

    // ptc5 default disabled, alt1 = PTC5/LLWU_P9
    PORTC_PCR5 = 0x100;

    GPIOC->DIR |= 1 << 5;

#if 1
    while (1) {
        GPIOC->SET = 1 << 5;
        for (int i = 0; i < (1 << 22); ++i)
            asm volatile("");
        GPIOC->CLR = 1 << 5;
        for (int i = 0; i < (1 << 24); ++i)
            asm volatile("");
    }
#else
    uint32_t z;
    while (1) {
        z += 2048;
        if (z > (z << 10))
            GPIOC->SET = 1 << 5;
        else
            GPIOC->CLR = 1 << 5;
    }
#endif
}

void * start[256] __attribute__ ((section (".start"), externally_visible));
void * start[256] = {
    [0] = (void*) 0x1ffffff0,
    [1] = go,
};

uint32_t flash_config[4] __attribute__ ((section (".fconfig"), externally_visible));
uint32_t flash_config[4] = { -1, -1, -1, -2 };
