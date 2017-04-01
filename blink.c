#include <stdint.h>

#define SIM_SCGC5 (*(volatile uint32_t *) 0x40048038)
#define PORTC_PCR5 (*(volatile uint32_t *) 0x4004B014)
#define GPIOC_PDDR (*(volatile uint32_t *) 0x400FF094)
#define GPIOC_PTOR (*(volatile uint32_t *) 0x400FF08C)

#define WDOG_UNLOCK (*(volatile uint16_t *) 0x4005200E)
#define WDOG_REFRESH (*(volatile uint16_t *) 0x4005200C)
#define WDOG_STCTRLH (*(volatile uint16_t *) 0x40052000)

void go()
{
    asm volatile ("cpsid i\n" ::: "memory");

    WDOG_UNLOCK = 0xc520;
    WDOG_UNLOCK = 0xd928;
    __asm__ volatile ("nop");
    __asm__ volatile ("nop");
    WDOG_STCTRLH = 0x10;

    SIM_SCGC5 |= 0x800;
    /* __asm__ volatile ("nop"); */
    /* __asm__ volatile ("nop"); */

    // ptc5 default disabled, alt1 = PTC5/LLWU_P9
    PORTC_PCR5 = 0x100;

    GPIOC_PDDR |= 1 << 5;

    while (1) {
        GPIOC_PTOR = 1 << 5;
        for (int i = 0; i < (1 << 20); ++i)
            asm volatile("");
    }
}

void * start[256] __attribute__ ((section (".start"), externally_visible));
void * start[256] = {
    [0] = (void*) 0x1ffffff0,
    [1] = go,
};

uint32_t flash_config[4] __attribute__ ((section (".fconfig"), externally_visible));
uint32_t flash_config[4] = { -1, -1, -1, -2 };
