
#include "k66.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static void go(void)
{
    interrupt_disable();
    // K66 gotcha: it comes out of reset with the WDOG enabled.  Turn it off
    // now.
    WDOG->UNLOCK = 0xc520;
    WDOG->UNLOCK = 0xd928;
    WDOG->STCTRLH = 0x10;

    // Zero the BSS.
    extern uint8_t bss_start[];
    extern uint8_t bss_end[];
    for (uint8_t * p = bss_start; p != bss_end; ++p)
        *p = 0;

    asm volatile("" ::: "memory");

    // Allow all the wierd modes (including HSRUN)...
    SMC->PMPROT = 0xaa;

    // Enable GPIOs on port C...
    SIM->CGC5 |= 0x800;

    // ptc5 default disabled, alt1 = PTC5/LLWU_P9
    PORTC_PCR[5] = 0x100;

    GPIOC->DIR |= 1 << 5;

    while (true) {
        GPIOC->TOGGLE = 1 << 5;
        for (volatile int i = 0; i < 1000000; ++i)
            ;
    }
}


void * __attribute__ ((section (".start"), externally_visible)) start[] = {
    (void*) 0x1ffffff0, go,
};

uint32_t flash_config[4] __attribute__ ((section (".fconfig"), externally_visible));
uint32_t flash_config[4] = { -1, -1, -1, -2 };
