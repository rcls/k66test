#ifndef K66_H_
#define K66_H_

#include <stdint.h>

typedef struct MCG_t {
    uint8_t C1;
    uint8_t C2;
    uint8_t C3;
    uint8_t C4;
    uint8_t C5;
    uint8_t C6;
    uint8_t S;
    uint8_t dummy7;
    uint8_t SC;
    uint8_t dummy9;
    uint8_t ATCVH;
    uint8_t ATCVL;
    uint8_t C7;
    uint8_t C8;
    uint8_t C9;
    uint8_t dummy15;
    uint8_t C11;
    uint8_t C12;
    uint8_t S2;
    uint8_t T3;
} MCG_t;

#define MCG ((volatile MCG_t *) 0x40064000)

// The MCG's C2 register bits configure the oscillator frequency range. See the
// OSC and MCG chapters for more details.
typedef struct OSC_t {
    uint8_t CR;
    uint8_t dummy1;
    uint8_t DIV;
} OSC_t;

#define OSC ((volatile OSC_t *) 0x40065000)

typedef struct SIM_t {
    uint32_t OPT1;
    uint32_t OPT1CFG;
    uint32_t USBPHYCTL;
    uint32_t dummy[1021];
    uint32_t dummy0;
    uint32_t OPT2;
    uint32_t dummy3;
    uint32_t OPT4;
    uint32_t OPT5;
    uint32_t dummy6;
    uint32_t OPT7;
    uint32_t OPT8;
    uint32_t OPT9;
    uint32_t DID;
    uint32_t CGC1;
    uint32_t CGC2;
    uint32_t CGC3;
    uint32_t CGC4;
    uint32_t CGC5;
    uint32_t CGC6;
    uint32_t CGC7;
    uint32_t CLKDIV1;
    uint32_t CLKDIV2;
    uint32_t FCFG1;
    uint32_t FCFG2;
    uint32_t UIDH;
    uint32_t UIDMH;
    uint32_t UIDML;
    uint32_t UIDL;
    uint32_t CLKDIV3;
    uint32_t CLKDIV4;
} SIM_t;
_Static_assert(sizeof(SIM_t) == 0x106c, "SIM_t size");

#define SIM ((volatile SIM_t *) 0x40047000)

#define SIM_SCGC5 (*(volatile uint32_t *) 0x40048038)
#define PORTC_PCR5 (*(volatile uint32_t *) 0x4004B014)

typedef struct PORT_t {
    uint32_t CR[32];
    uint32_t GPCLR;                      // "Control Low Register"
    uint32_t GPCHR;
    uint64_t dummy0[6];
    uint32_t ISFR;                      // Interrupt status
    uint32_t dummy1[7];
    uint32_t DFER;
    uint32_t DFCR;
    uint32_t DFWR;
} PORT_t;

typedef struct GPIO_t {
    uint32_t OUT;
    uint32_t SET;
    uint32_t CLR;
    uint32_t TOGGLE;
    const uint32_t IN;
    uint32_t DIR;
    uint32_t dummy[10];
} GPIO_t;

#define GPIOA ((volatile GPIO_t *) 0x400ff000)
#define GPIOB ((volatile GPIO_t *) 0x400ff040)
#define GPIOC ((volatile GPIO_t *) 0x400ff080)
#define GPIOD ((volatile GPIO_t *) 0x400ff0c0)
#define GPIOE ((volatile GPIO_t *) 0x400ff100)


#define GPIOC_PDDR (*(volatile uint32_t *) 0x400FF094)
#define GPIOC_PTOR (*(volatile uint32_t *) 0x400FF08C)

#define WDOG_UNLOCK (*(volatile uint16_t *) 0x4005200E)
#define WDOG_REFRESH (*(volatile uint16_t *) 0x4005200C)
#define WDOG_STCTRLH (*(volatile uint16_t *) 0x40052000)

typedef struct WDOG_t {
    uint16_t STCTRLH;
    uint16_t STCTRLL;
    uint16_t TOVALH;
    uint16_t TOVALL;
    uint16_t WINH;
    uint16_t WINL;
    uint16_t REFRESH;
    uint16_t UNLOCK;
    uint16_t TMROUTH;
    uint16_t TMROUTL;
    uint16_t RSTCNT;
    uint16_t PRESC;
} WDOG_t;

#define WDOG ((volatile WDOG_t *) 0x40052000)

#endif
