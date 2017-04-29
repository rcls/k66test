#ifndef K66_H_
#define K66_H_

#include <stdint.h>

#define interrupt_disable() asm volatile ("cpsid i\n" ::: "memory");
#define interrupt_enable() asm volatile ("cpsie i\n" ::: "memory");
#define interrupt_wait() asm volatile ("wfi\n");
#define interrupt_wait_go() do {                \
        interrupt_wait();                       \
        interrupt_enable();                     \
        interrupt_disable();                    \
    } while (0)

typedef unsigned char __attribute__((aligned(4))) reg8_t;
typedef union reg8_u {
    unsigned char b;
    uint32_t u32;
} reg8_u;

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

// The MCG's C2 register bits configure the oscillator frequency range. See the
// OSC and MCG chapters for more details.
typedef struct OSC_t {
    uint8_t CR;
    uint8_t dummy1;
    uint8_t DIV;
} OSC_t;

typedef struct SMC_t {
    uint8_t PMPROT;
    uint8_t PMCTRL;
    uint8_t STOPCTRL;
    uint8_t PMSTAT;
} SMC_t;

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

typedef struct FMC_t {
    uint32_t PFAPR;
    uint32_t PFB01CR;
    uint32_t PFB23CR;
    uint32_t dummy[63];

    uint32_t TAG[4][4];
    uint32_t dummy2[48];

    uint32_t DATA[4][4][4];
} FMC_t;

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


typedef struct MPU_t {
    uint32_t CESR;
    uint32_t dummy1[3];
    struct {
        uint32_t address;
        uint32_t detail;
    } ERROR[5];
    uint32_t dummy2[0xf2];
    struct {
        uint32_t SRTADDR, ENDADDR, RIGHTS, PID;
    } RGD[12];
    uint32_t dummy3[0xd0];
    uint32_t AAC[12];
} MPU_t;

typedef struct USBFS_t {
    const reg8_t PERID;
    const reg8_t IDCOMP;
    const reg8_t REV;
    const reg8_t ADDINFO;
    reg8_t OTGISTAT;
    reg8_t OTGICR;
    reg8_t OTGSTAT;
    reg8_t OTGCTL;

    uint32_t dummy8[24];

    reg8_t ISTAT;
    reg8_t INTEN;
    reg8_t ERRSTAT;
    reg8_t ERREN;
    const reg8_t STAT;
    reg8_t CTL;
    reg8_t ADDR;
    reg8_t BDTPAGE1;
    reg8_t FRMNUML;
    reg8_t FRMNUMH;
    reg8_t TOKEN;
    reg8_t SOFTHLD;
    reg8_t BDTPAGE2;
    reg8_t BDTPAGE3;

    uint32_t dummy[2];

    reg8_u ENDPT[16];

    reg8_t USBCTRL;
    const reg8_t OBSERVE;
    reg8_t CONTROL;
    reg8_t USBTRC0;
    reg8_t USBFRMADJUST;
/*
4007_2140 USB Clock recovery control (USB0_CLK_RECOVER_CTRL) 8 R/W 00h 51.5.29/
4007_2144 IRC48M oscillator enable register (USB0_CLK_RECOVER_IRC_EN) 8 R/W 01h 51.5.30/
4007_2154 Clock recovery combined interrupt enable (USB0_CLK_RECOVER_INT_EN) 8 R/W 10h 51.5.31/
4007_215C Clock recovery separated interrupt status (USB0_CLK_RECOVER_INT_STATUS)
*/
} USBFS_t;

typedef struct BDT_item_t {
    uint32_t flags;
    void * address;
} BDT_item_t;

typedef struct BDT_ep_t {
    BDT_item_t rx[2];
    BDT_item_t tx[2];
} BDT_ep_t;


typedef struct NVIC_t {
    uint32_t dummy;
    uint32_t ICTR;
    uint32_t dummy0[62];
    uint32_t ISER[32];                  // Set enable.
    uint32_t ICER[32];                  // Clear enable.
    uint32_t ISPR[32];                  // Set pending.
    uint32_t ICPR[32];                  // Clear pending.
    uint32_t IABR[32];                  // Active bit register.
    uint32_t dummy1[32];
    // Interrupt priority registers.
    union {
        uint32_t IPRw[256];
        uint8_t IPR[1024];
    };
    uint32_t dummy2[7 * 64];
    uint32_t STIR;                      // Software Triggered Interrupt.
} NVIC_t;


typedef struct VECTORS_t {
    void * exceptions[16];
    void * interrupts[256 - 16];
} VECTORS_t;

#define MPU ((volatile MPU_t *) 0x4000D000)
#define FMC ((volatile FMC_t *) 0x4001F000)

#define PORTC_PCR ((volatile uint32_t *) 0x4004B000)

#define WDOG  ((volatile WDOG_t *) 0x40052000)

#define MCG ((volatile MCG_t *) 0x40064000)
#define OSC ((volatile OSC_t *) 0x40065000)

#define SIM   ((volatile SIM_t *) 0x40047000)
#define USB0  ((volatile USBFS_t *) 0x40072000)
#define SMC   ((volatile SMC_t *) 0x4007e000)

#define GPIOA ((volatile GPIO_t *) 0x400ff000)
#define GPIOB ((volatile GPIO_t *) 0x400ff040)
#define GPIOC ((volatile GPIO_t *) 0x400ff080)
#define GPIOD ((volatile GPIO_t *) 0x400ff0c0)
#define GPIOE ((volatile GPIO_t *) 0x400ff100)

#define NVIC ((volatile NVIC_t *) 0xE000E000)

_Static_assert((unsigned) &MPU->ERROR[0].address == 0x4000d010, "MPU err");
_Static_assert((unsigned) &MPU->RGD[0].SRTADDR == 0x4000d400, "MPU region");
_Static_assert((unsigned) &MPU->AAC[0] == 0x4000d800, "MPU alt");

_Static_assert((unsigned) PORTC_PCR == 0x4004b000, "PORTC pcr");
_Static_assert((unsigned) &PORTC_PCR[5] == 0x4004b014, "PORTC pcr5");

_Static_assert(&MCG->C6 == (void *) 0x40064005, "MCG C6");
_Static_assert(&MCG->S == (void *) 0x40064006, "MCG S");

_Static_assert((unsigned) &USB0->ISTAT == 0x40072080, "USB0 istat");
_Static_assert((unsigned) &USB0->ENDPT[0] == 0x400720c0, "USB0 ep0");
_Static_assert((unsigned) &USB0->USBCTRL == 0x40072100, "USB0 usbctrl");

_Static_assert((unsigned) &SIM->OPT2 == 0x40048004, "SIM opt2");
_Static_assert((unsigned) &SIM->CLKDIV2 == 0x40048048, "SIM clkdiv");
_Static_assert((unsigned) &SIM->CGC4 == 0x40048034, "SIM cgc4");

_Static_assert((unsigned) &NVIC->ISER == 0xe000e100, "NVIC ISER");
_Static_assert((unsigned) &NVIC->IABR == 0xe000e300, "NVIC IABR");
_Static_assert((unsigned) &NVIC->IPR  == 0xe000e400, "NVIC IPR");
_Static_assert((unsigned) &NVIC->STIR == 0xe000ef00, "NVIC STIR");

enum {
    i_DMA = 0,                          // Separate per channel.
    i_DMA_error = 16,
    i_MCM,
    i_FLASH_complete,
    i_FLASH_collide,
    i_low_voltage,
    i_LLWU,
    i_WDOG_EWM,
    i_RNG,
    i_I2C0,
    i_I2C1,
    i_SPI0,
    i_SPI1,
    i_I2S0_tx,
    i_I2S0_rx,
    i_30,
    i_UART0,
    i_UART0_error,
    i_UART1,
    i_UART1_error,
    i_UART2,
    i_UART2_error,
    i_UART3,
    i_UART3_error,
    i_ADC0,
    i_CMP0,
    i_CMP1,
    i_FTM0,
    i_FTM1,
    i_FTM2,
    i_CMT,
    i_RTC_alarm,
    i_RTC_seconds,
    i_PIT0,
    i_PIT1,
    i_PIT2,
    i_PIT3,
    i_PDB,
    i_USBFS,
    i_USBFS_DCD,
    i_55,
    i_DAC0,
    i_MCG,
    i_LPT,
    i_PORTA,
    i_PORTB,
    i_PORTC,
    i_PORTD,
    i_PORTE,
    i_software,
    i_SPI2,
    i_UART4,
    i_UART4_error,
    i_68,
    i_69,
    i_CMP2,
    i_FTM3,
    i_DAC1,
    i_ADC1,
    i_I2C2,
    i_CAN0_MSG,
    i_CAN0_bus_off,
    i_CAN0_error,
    i_CAN0_tx_warn,
    i_CAN0_rx_warn,
    i_CAN0_wake_up,
    i_ETH_timer,
    i_ETH_tx,
    i_ETH_rx,
    i_ETH_error,
    i_LPUART0,
    i_TSI0,
    i_TPM1,
    i_TPM2,
    i_USBHS,
    i_I2C3,
    i_CMP3,
    i_USBHS_OTG,
    i_CAN1_MSG,
    i_CAN1_bus_off,
    i_CAN1_error,
    i_CAN1_tx_warn,
    i_CAN1_rx_warn,
    i_CAN1_wake_up,
};

_Static_assert(i_USBFS == 53, "i USBFS");

#endif
