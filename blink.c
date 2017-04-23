
#include "k66.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

volatile BDT_ep_t __attribute__((aligned(512))) bdt[16];

#define STAT_TO_BDT(n) ((volatile BDT_item_t *) ((char *) bdt + 2 * (n)))

// f055 4b36

enum string_descs_t {
    sd_lang,
    sd_ralph,
    sd_blink,
    sd_0001,
    sd_jumper,
};


static const uint16_t string_lang[2] = u"\x0304\x0409";
static const uint16_t string_ralph[6] = u"\x030c""Ralph";
static const uint16_t string_blink[6] = u"\x030c""Blink";
static const uint16_t string_0001[5] = u"\x030a""1337";
static const uint16_t string_jumper[7] = u"\x030e""Jumper";

static const uint16_t * const string_descriptors[] = {
    string_lang,
    [sd_ralph] = string_ralph,
    [sd_blink] = string_blink,
    [sd_0001] = string_0001,
    [sd_jumper] = string_jumper,
};

#define DEVICE_DESCRIPTOR_SIZE 18
static const uint8_t device_descriptor[] = {
    DEVICE_DESCRIPTOR_SIZE,
    1,                                  // type: device
    0, 2,                               // bcdUSB.
    0,                                  // class - compound.
    0,                                  // subclass.
    0,                                  // protocol.
    64,                                 // Max packet size.
    0x55, 0xf0,                         // Vendor-ID.
    '6', 'K',                           // Device-ID.
    0x34, 0x12,                         // Revision number.
    sd_ralph,                           // Manufacturer string index.
    sd_blink,                           // Product string index.
    sd_0001,                            // Serial number string index.
    1                                   // Number of configurations.
};
_Static_assert (DEVICE_DESCRIPTOR_SIZE == sizeof (device_descriptor),
                "device_descriptor size");

enum usb_interfaces_t {
    usb_intf_jumper,
    usb_num_intf
};



#define CONFIG_DESCRIPTOR_SIZE (9 + 9 + 7 + 7)
static const uint8_t config_descriptor[] = {
    // Config.
    9,                                  // length.
    2,                                  // type: config.
    CONFIG_DESCRIPTOR_SIZE & 0xff,      // size.
    CONFIG_DESCRIPTOR_SIZE >> 8,
    usb_num_intf,                       // num interfaces.
    1,                                  // configuration number.
    0,                                  // string descriptor index.
    0x80,                               // attributes, not self powered.
    250,                                // current (500mA).
    // Interface (jumper).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_jumper,                    // interface number.
    0,                                  // alternate setting.
    2,                                  // number of endpoints.
    0xff,                               // interface class (vendor specific).
    'S',                                // interface sub-class.
    'S',                                // protocol.
    sd_jumper,                          // interface string index.
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    3,                                  // OUT 3.
    0x2,                                // bulk
    64, 0,                              // packet size
    0,
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    0x83,                               // IN 3.
    0x2,                                // bulk
    64, 0,                              // packet size
    0,

};
_Static_assert (CONFIG_DESCRIPTOR_SIZE == sizeof (config_descriptor),
                "config_descriptor size");

static void hang(void)
{
    while (1) {
        GPIOC->SET = 1 << 5;
        for (int i = 0; i < (1 << 22); ++i)
            asm volatile("");
        GPIOC->CLR = 1 << 5;
        for (int i = 0; i < (1 << 24); ++i)
            asm volatile("");
    }
}

static bool set_address;
static uint8_t address;

void go(void)
{
    asm volatile ("cpsid i\n" ::: "memory");

    WDOG->UNLOCK = 0xc520;
    WDOG->UNLOCK = 0xd928;
    __asm__ volatile ("nop");
    __asm__ volatile ("nop");
    WDOG->STCTRLH = 0x10;

    extern uint8_t bss_start[];
    extern uint8_t bss_end[];
    for (uint8_t * p = bss_start; p != bss_end; ++p)
        *p = 0;

    asm volatile("" ::: "memory");

    // Allow all the wierd modes (including HSRUN)...
    SMC->PMPROT = 0xaa;

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

    // Now configure the PLL for 168 Mhz = 8 x 21.  So /2 and *21.
    MCG->C6 = 5;                        // *21.
    // MCG->C5 = 0x61;                     // Enable, Stop-en, /2
    // AFAICS we're running at half the speed I thought we were...
    MCG->C5 = 0x60;                     // Enable, Stop-en, /2

    // Wait for lock...
    _Static_assert(&MCG->S == (void *) 0x40064006, "MCG S");
    while (~MCG->S & 0x40);

    // Now select PLLS from PLL not FLL.  Keep *21.
    _Static_assert(&MCG->C6 == (void *) 0x40064005, "MCG C6");
    MCG->C6 = 0x45;

    // Wait for PLLS to take effect...
    while (~MCG->S & 0x20);

    // The flash clock has a max. of 28MHz.  Set it to /6.
    // The bus clock has a max. of 60MHz, set it to /3.  Ditto flexbus even
    // though we're not using it.
    SIM->CLKDIV1 = 0x2250000;

    // Enter HSRUN...
    SMC->PMCTRL = 0x60;
    while (SMC->PMSTAT != 0x80);

    // Now switch CLKS to the PLL...
    MCG->C1 = MCG->C1 & ~0xc0;

    // And wait for it to take effect.
    while ((MCG->S & 0xc) != 0xc);

    // Just turn off the MPU for now...
    MPU->CESR &= ~1;

    // Enable USB access to flash.
    FMC->PFAPR = 0xffff;

    // ptc5 default disabled, alt1 = PTC5/LLWU_P9
    PORTC_PCR[5] = 0x100;

    GPIOC->DIR |= 1 << 5;

    // Set-up USB clk src...
    // USBSRC = PLLFLLSEL, PLLFLLSEL = PLL.
    SIM->OPT2 = 0x51000;
    // Set the USB clk divider to /3.5 = 48MHz
    SIM->CLKDIV2 = 13;

    // Enable the USB clock.
    SIM->CGC4 |= 1 << 18;

    // BDT base address..
    unsigned b = (unsigned) &bdt;
    USB0->BDTPAGE1 = b >> 8;
    USB0->BDTPAGE2 = b >> 16;
    USB0->BDTPAGE3 = b >> 24;

    USB0->ADDR = 0;

    // Set up endpoint zero rx...
    // Disable DTS on these, that way the same BDT can process the SETUP token
    // and the OUT token used for a control ACK.
    static volatile uint32_t setup[16];
    bdt[0].rx[0].address = (void *) &setup;
    bdt[0].rx[0].flags = 0x400088;          // 8 bytes, USBFS own, DTS.
    bdt[0].rx[1].address = (void *) &setup;
    bdt[0].rx[1].flags = 0x400088;          // 8 bytes, USBFS own, DTS.

    // Let's setup tx also...
    bdt[0].tx[0].address = (void *) &setup;
    bdt[0].tx[0].flags = 0;             // 8 bytes, USBFS own, DTS.
    bdt[0].tx[1].address = (void *) &setup;
    bdt[0].tx[1].flags = 0;             // 8 bytes, USBFS own, DTS.

    // DP pullup...
    USB0->OTGCTL = 0x82;                // DPHIGH + OTGEN (automatic).
    USB0->CONTROL = 0x10;               // non-OTG pull-up...

    USB0->ENDPT[0].b = 0x0d;              // Endpoint is control.

    // Release from suspend, weak pull downs.
    USB0->USBCTRL = 0;

    // Unmask TOKDONE and RST and SOF
    USB0->INTEN = 9 + 4;

    // OTG 1ms int.
    // USB0->OTGICR = 0x40;

    USB0->CTL = 1;                      // Enable...
    USB0->CTL = 3;                      // Reset odd/even.
    USB0->CTL = 1;                      // Enable...

    bool next_tx = 0;

    while (1) {
        // Just in case it stalls...
        //USB0->ENDPT[0].b = 0x0d;              // Endpoint is control.

        //GPIOC->OUT = (USB0->FRMNUMH & 1) ? 1 << 5 : 0;
        /* if (++toggle == 1048576) { */
        /*     toggle = 0; */
        /*     GPIOC->TOGGLE = 1 << 5; */
        /* } */
        int istat = USB0->ISTAT;
        if (istat & 1) {
            bdt[0].rx[0].address = (void *) &setup;
            bdt[0].rx[0].flags = 0x400088;          // 8 bytes, USBFS own, DTS.
            bdt[0].rx[1].address = (void *) &setup;
            bdt[0].rx[1].flags = 0x4000c8;          // 8 bytes, USBFS own, DTS.

            USB0->ENDPT[0].b = 0x0d;              // Endpoint is control.
            USB0->ADDR = 0;
            USB0->CTL = 3;                      // Reset odd/even.
            USB0->CTL = 1;                      // Enable...
            USB0->ISTAT = istat;
            next_tx = 0;
            continue;
        }

        if (istat & 4) {
            USB0->ISTAT = 4;
            static int toggle;
            if (++toggle == 100) {
                toggle = 0;
                //GPIOC->TOGGLE = 1 << 5;
            }
        }

        if ((istat & 8) == 0)
            continue;

        int done = USB0->STAT;
        USB0->ISTAT = 8;

        // If it's ep 0 TX, update the next_tx flag...
        if ((done & ~4) == 8) {
            // GPIOC->SET = 1 << 5;
            next_tx = !(done & 4);
            if (set_address)
                USB0->ADDR = address;
            set_address = false;
            continue;
        }

        // If it's not ep 0 RX, don't care...
        if ((done & ~4) != 0)
            continue;

        unsigned setup0 = setup[0];
        unsigned setup1 = setup[1];

        // Resume the BDT...
        unsigned flags = STAT_TO_BDT(done)->flags;
        STAT_TO_BDT(done)->flags = 0x4000c8;
        STAT_TO_BDT(done ^ 4)->flags = 0x400088;

        // If it's not a setup, ignore it.
        if ((flags & 0x3c) != 0xd * 4) {
            hang();
            continue;
        }

        const void * response_data = NULL;
        int response_length = -1;

        switch (setup0 & 0xffff) {
        case 0x0680:                    // Get descriptor.
            switch (setup0 >> 24) {
            case 1:
                response_data = device_descriptor;
                response_length = DEVICE_DESCRIPTOR_SIZE;
                break;
            case 2:                         // Configuration.
                response_data = config_descriptor;
                response_length = CONFIG_DESCRIPTOR_SIZE;
                break;
            case 3: {                        // String.
                unsigned index = (setup0 >> 16) & 255;
                if (index < sizeof string_descriptors / 4) {
                    response_data = string_descriptors[index];
                    response_length = *string_descriptors[index] & 0xff;
                }
                break;
            }
            }
            break;
        case 0x0500:                        // Set address.
            // GPIOC->SET = 1 << 5;
            //USB0->ADDR = (setup0 >> 16) & 255;
            address = (setup0 >> 16) & 255;
            set_address = true;
            response_length = 0;
            break;

        case 0x0900:                // Set configuration.
            response_length = 0;
            break;

        case 0x0b01:                // Set interface
            response_length = 0;
            break;
        }

        // Don't stall for now...
        if (response_length >= 0) {
        //if (response_data) {
            // This is a data1...
            if ((setup1 >> 16) < response_length)
                response_length = setup1 >> 16;

            bdt[0].tx[next_tx].address = (void *) response_data;
            bdt[0].tx[next_tx].flags = (response_length << 16) + 0xc8;
            // next_tx = !next_tx;
        }
        else {
            hang();
        }

        // Resume the EP, it pauses on setup.
        USB0->CTL &= ~0x20;
    }
}

void * start[256] __attribute__ ((section (".start"), externally_visible));
void * start[256] = {
    [0] = (void*) 0x1ffffff0,
    [1] = go,
};

uint32_t flash_config[4] __attribute__ ((section (".fconfig"), externally_visible));
uint32_t flash_config[4] = { -1, -1, -1, -2 };
