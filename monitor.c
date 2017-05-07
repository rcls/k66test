
#include "k66.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void monitor(void);

volatile BDT_ep_t __attribute__((aligned(512))) bdt[16];

#define STAT_TO_BDT(n) ((volatile BDT_item_t *) ((char *) bdt + 2 * (n)))

enum string_descs_t {
    sd_lang,
    sd_ralph,
    sd_blink,
    sd_1337,
    sd_monkey,
};

static const uint16_t string_lang[2] = u"\x0304\x0409";
static const uint16_t string_ralph[6] = u"\x030c""Ralph";
static const uint16_t string_blink[6] = u"\x030c""Blink";
static const uint16_t string_1337[5] = u"\x030a""1337";
static const uint16_t string_monkey[7] = u"\x030e""Monkey";

static const uint16_t * const string_descriptors[] = {
    string_lang,
    [sd_ralph] = string_ralph,
    [sd_blink] = string_blink,
    [sd_1337] = string_1337,
    [sd_monkey] = string_monkey,
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
    sd_1337,                            // Serial number string index.
    1                                   // Number of configurations.
};
_Static_assert (DEVICE_DESCRIPTOR_SIZE == sizeof (device_descriptor),
                "device_descriptor size");

enum usb_interfaces_t {
    usb_intf_monkey,
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
    // Interface (monkey).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_monkey,                    // interface number.
    0,                                  // alternate setting.
    2,                                  // number of endpoints.
    0xff,                               // interface class (vendor specific).
    'S',                                // interface sub-class.
    'S',                                // protocol.
    sd_monkey,                          // interface string index.
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    1,                                  // OUT 3.
    0x2,                                // bulk
    64, 0,                              // packet size
    0,
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    0x81,                               // IN 3.
    0x2,                                // bulk
    64, 0,                              // packet size
    0,
};
_Static_assert (CONFIG_DESCRIPTOR_SIZE == sizeof (config_descriptor),
                "config_descriptor size");

static void __attribute__((noreturn)) hang(void)
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
static volatile uint32_t setup[16];
static bool next_ep0_tx;

// Up to two live output buffers + 64 bytes pending.
static char * monkey_tx_next;
static char * monkey_tx_write;
static bool monkey_tx_odd;
static char monkey_tx[192];

static struct {
    uint8_t * next;
    uint8_t * end;
} monkey_recv_pos[2];

static uint8_t monkey_recv[128];
static bool monkey_rx_next;             // Next to schedule.

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))


// Should be called with interrupts disabled.
static void monkey_tx_schedule(void)
{
    while (bdt[1].tx[monkey_tx_odd].flags & 0x80)
        interrupt_wait_go();

    // For the monkey we keep even/odd sync'd with DATA0/DATA1.
    bdt[1].tx[monkey_tx_odd].address = monkey_tx_next;
    int len = monkey_tx_write - monkey_tx_next;
    if (len > 64)
        hang();                         // Should never happen.
    bdt[1].tx[monkey_tx_odd].flags = (len << 16) + 0x88 + (monkey_tx_odd << 6);
    monkey_tx_odd = !monkey_tx_odd;

    if (monkey_tx_write - monkey_tx >= 128)
        monkey_tx_write = monkey_tx;

    monkey_tx_next = monkey_tx_write;
}


static void monkey_start(void)
{
    GPIOC->SET = 1 << 5;

    // Stop the TX endpoint...  Any old buffered data is gone gone gone.
    bdt[1].tx[0].flags = 0;
    bdt[1].tx[1].flags = 0;
    monkey_tx_odd = false;

    // Work out which way round to schedule the buffers...
    uintptr_t last = (uintptr_t) monkey_recv_pos[1].next;
    if (last == 0)
        last = (uintptr_t) monkey_recv_pos[0].next;
    if (last < (uintptr_t) monkey_recv + 64) {
        bdt[1].rx[0].address = monkey_recv + 64;
        bdt[1].rx[1].address = monkey_recv;
    }
    else {
        bdt[1].rx[0].address = monkey_recv;
        bdt[1].rx[1].address = monkey_recv + 64;
    }

    // Now work out what to schedule...
    if (monkey_recv_pos[1].next == NULL)
        bdt[1].rx[0].flags = 0x400088;
    else
        bdt[1].rx[0].flags = 0;

    if (monkey_recv_pos[0].next == NULL)
        bdt[1].rx[1].flags = 0x4000c8;
    else
        bdt[1].rx[1].flags = 0;

    if (monkey_recv_pos[0].next != NULL && monkey_recv_pos[1].next == NULL)
        monkey_rx_next = 0;
    else
        monkey_rx_next = 1;

    // Start the EP.
    USB0->ENDPT[1].b = 0x1d;
}


void putchar(char c)
{
    *monkey_tx_write++ = c;
    if (monkey_tx_write - monkey_tx_next >= 64) {
        interrupt_disable();
        monkey_tx_schedule();
        interrupt_enable();
    }
}

static int ungetbyte;

static void ungetchar(int byte)
{
    ungetbyte = byte;
}

int getchar(void)
{
    if (ungetbyte >= 0) {
        int r = ungetbyte;
        ungetbyte = -1;
        return r;
    }

    // FIXME - fold the two update cases...  Then we should leave ourselves
    // with always a valid buffer for ungetc...
    uint8_t * n = monkey_recv_pos->next;
    if (n == NULL) {
        interrupt_disable();
        if (monkey_tx_write != monkey_tx_next)
            monkey_tx_schedule();       // Flush...
        while (monkey_recv_pos->next == NULL)
            interrupt_wait_go();
        n = monkey_recv_pos->next;
        interrupt_enable();
    }
    int r = *n++;
    if (n != monkey_recv_pos->end) {
        monkey_recv_pos->next = n;
        return r;
    }

    interrupt_disable();
    monkey_recv_pos[0] = monkey_recv_pos[1];
    monkey_recv_pos[1].next = NULL;
    monkey_recv_pos[1].end = NULL;

    // Reschedule the end-point...
    bdt[1].rx[monkey_rx_next].flags = 0x400088 + (monkey_rx_next << 6);
    monkey_rx_next = !monkey_rx_next;
    interrupt_enable();

    return r;
}


static void usb_reset(void)
{
    GPIOC->CLR = 1 << 5;

    // USB0->CTL = 2;                      // Disable, reset odd/even.
    USB0->CTL = 3;                      // Disable, reset odd/even.
    bdt[0].rx[0].address = (void *) &setup;
    bdt[0].rx[0].flags = 0x400088;      // 64 bytes, USBFS own, DTS.
    bdt[0].rx[1].address = (void *) &setup;
    bdt[0].rx[1].flags = 0;

    next_ep0_tx = 0;

    USB0->ENDPT[0].b = 0x0d;            // Endpoint is control.
    USB0->ADDR = 0;

    monkey_tx_next = monkey_tx;
    monkey_tx_write = monkey_tx;

    USB0->CTL = 1;                      // Enable...
}


static void usb_monkey_rx(int done)
{
    int index = (monkey_recv_pos->next != NULL);
    uint8_t * buffer = STAT_TO_BDT(done)->address;
    unsigned length = STAT_TO_BDT(done)->flags >> 16;

    monkey_recv_pos[index].next = buffer;
    monkey_recv_pos[index].end = buffer + length;
}


static void usb_ep0_rx(int done)
{
    // Resume the BDT...
    unsigned flags = STAT_TO_BDT(done)->flags;
    STAT_TO_BDT(done ^ 4)->flags = 0x400088;

    // If it's not a setup, ignore it.
    if ((flags & 0x3c) != 0xd * 4)
        hang();

    const void * response_data = NULL;
    int response_length = -1;

    unsigned setup0 = setup[0];
    switch (setup0 & 0xffff) {
    case 0x0680:                        // get descriptor.
        switch (setup0 >> 24) {         // Type
        case 1:                         // Device descriptor.
            response_data = device_descriptor;
            response_length = DEVICE_DESCRIPTOR_SIZE;
            break;
        case 2:                         // Configuration.
            response_data = config_descriptor;
            response_length = CONFIG_DESCRIPTOR_SIZE;
            break;
        case 3: {                       // String.
            unsigned index = (setup0 >> 16) & 255;
            if (index < sizeof string_descriptors / 4) {
                response_data = string_descriptors[index];
                response_length = *string_descriptors[index] & 0xff;
            }
            break;
        }
        }
        break;

    case 0x0080:                        // Get status.
        response_data = "\0";
        response_length = 2;
        break;

    case 0x0500:                        // Set address.
        // GPIOC->SET = 1 << 5;
        address = (setup0 >> 16) & 255;
        set_address = true;
        response_length = 0;
        break;

    case 0x0900:                        // Set configuration.
        // We only have one configuration so we just set it up by default.
        // FIXME - this resets data toggle?  FIXME - what happens to already
        // sechduled buffers?
        USB0->CTL = 3;
        USB0->CTL = 1;
        monkey_start();
        response_length = 0;
        next_ep0_tx = 0;
        break;

    case 0x0b01:         // Set interface.
        // We only have one interface, we set it up as default.
        response_length = 0;
        break;
    }

    if (response_length >= 0) {
        unsigned setup1 = setup[1];
        if ((setup1 >> 16) < response_length)
            response_length = setup1 >> 16;

        // For the setup we toggle next_ep0_tx when it's acked... just in case
        // the host does not read our response.
        bdt[0].tx[next_ep0_tx].address = (void *) response_data;
        bdt[0].tx[next_ep0_tx].flags = (response_length << 16) + 0xc8;
    }
    else {
        bdt[0].tx[next_ep0_tx].flags = 0xcc; // Stall.
    }

    // Resume the device, it pauses on setup.
    USB0->CTL &= ~0x20;
}


static void usb_interrupt(void)
{
    int istat = USB0->ISTAT;
    if (istat & 1) {                    // RST
        usb_reset();
        USB0->ISTAT = istat;
        return;
    }

    if (istat & 128) {                  // STALL
        if (istat & 8)
            // FIXME - this is now expected...
            hang();                     // Unexpected.
        if (~USB0->ENDPT[0].b & 2)
            hang();
        USB0->ENDPT[0].b &= ~2;
        USB0->ISTAT = 128;
    }

    if ((istat & 8) == 0)               // Token done
        return;

    int done = USB0->STAT;
    USB0->ISTAT = 8;

    switch (done & ~4) {
    case 0:                             // EP 0 RX.
        usb_ep0_rx(done);
        break;

    case 8:                             // EP 0 TX.
        if (set_address) {
            USB0->ADDR = address;
            set_address = false;
        }
        next_ep0_tx = !(done & 4);
        break;

    case 16:                            // EP 1 RX.
        usb_monkey_rx(done);
        break;

    case 24:                            // EP 1 TX.
        // All the work is done from monkey_tx_schedule()...
        // usb_monkey_tx(done);
        break;
    }
}


static void puts(const char * s)
{
    for (; *s; ++s)
        putchar(*s);
}


static void puthex(unsigned n, unsigned len)
{
    unsigned l = len * 4;
    do {
        l -= 4;
        unsigned nibble = (n >> l) & 15;
        if (nibble >= 10)
            nibble += 'a' - '0' - 10;
        putchar(nibble + '0');
    }
    while (l);
}


static unsigned getchar_skip_space(void)
{
    unsigned b;
    do
        b = getchar();
    while (b == ' ');
    return b;
}


static unsigned get_hex(unsigned max)
{
    unsigned result = 0;
    unsigned d = getchar_skip_space();
    for (unsigned i = 0; i < max; ++i) {
        unsigned c = d;
        if (c >= 'a')
            c -= 32;                    // Upper case.
        c -= '0';
        if (c >= 10) {
            c -= 'A' - '0' - 10;
            if (c < 10 || c > 15)
                break;
        }
        result = result * 16 + c;
        d = getchar();
    }
    ungetchar(d);
    return result;
}


static void _Noreturn command_abort(const char * s)
{
    puts(s);
    unsigned sp = 0x1ffffff0;
    unsigned m = (unsigned) monitor;
    asm volatile ("mov sp,%0\n" "bx %1\n" :: "r" (sp), "r" (m));
    __builtin_unreachable();
}


static void _Noreturn command_error(const char * s)
{
    while (getchar() != '\n');
    command_abort(s);
}


static void command_end()
{
    if (getchar_skip_space() != '\n')
        command_error("? Crap on end of line.\n");
}


static void * get_address(void)
{
    void * r = (void *) get_hex(8);
    command_end();
    return r;
}


static void command_go(void)
{
    void * v = get_address();
    puts("Go\n");

    interrupt_disable();
    if (monkey_tx_write != monkey_tx_next)
        monkey_tx_schedule();       // Flush...

    while (bdt[1].tx[0].flags & 0x80 || bdt[1].tx[1].flags & 0x80)
        interrupt_wait_go();
    interrupt_enable();

    // Now reset...
    SystemRegisterFile->w[0] = (unsigned) v;

    while (true)
        SCB->AIRCR = 0x05FA0004;
}


static void command_ping(void)
{
    putchar('P');
    unsigned char c;
    do {
        c = getchar();
        putchar(c);
    }
    while (c != '\n');
}


static void command_read(void)
{
    uint8_t * address = get_address();

    puthex((unsigned) address, 8);
    char sep = ':';
    for (int i = 0; i != 16; ++i) {
        putchar(sep);
        puthex(address[i], 2);
        sep = ' ';
    }
    putchar('\n');
}


static void command_write(void)
{
    uint8_t * address = (uint8_t *) get_hex(8);
    uint8_t bytes[16];
    unsigned n = 0;
    for (; n != sizeof bytes; ++n) {
        int c = getchar_skip_space();
        if (c == '\n')
            goto do_write;
        ungetchar(c);
        bytes[n] = get_hex(2);
    }

    command_end();

do_write:
    // Memory.
    for (unsigned i = 0; i < n; ++i)
        address[i] = bytes[i];
}


static void command(void)
{
    unsigned c = getchar_skip_space();
    if (c == 'R') {
        command_read();
        return;
    }
    else if (c == 'P') {
        command_ping();
        return;
    }
    else if (c == 'W')
        command_write();
    else if (c == 'G')
        command_go();
    else if (c == '\n')
        return;
    else
        command_error("Unknown command\n");

    putchar(c);
    putchar('\n');
}


void _Noreturn monitor(void)
{
    while (true)
        command();
}


static void go(void)
{
    // If SystemRegisterFile indicates a boot target, jump to it...
    if (SystemRegisterFile->w[0]) {
        uint32_t * vt = (uint32_t *) SystemRegisterFile->w[0];
        SystemRegisterFile->w[0] = SystemRegisterFile->w[1];
        SCB->VTOR = vt;
        asm volatile ("mov sp,%0\n" "bx %1\n" :: "r" (vt[0]), "r" (vt[1]));
        __builtin_unreachable();
    }

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

    // Set RANGE=very high frequency, HGO=low gain, EREFS=oscillator ...
    MCG->C2 = 0x24;
    // Set the OSC to enabled, run in stop, 18pf load.
    OSC->CR = 0xa9;

    // Wait for the OSCINIT0 bit to come through...
    while (~MCG->S & 2);

    // Switch the MCGOUTCLK to CLKS=2, erefclk.
    MCG->C1 = (MCG->C1 & ~0xc0) | 0x80;

    // Wait for the switch...
    while ((MCG->S & 0xc) != 8);

    // Switch the FLL clock to the eclk, /512.  Why are we fluffing with the
    // FLL, we are running off the XTAL by now?
    MCG->C1 = 0xa0;
    // Wait for IREF to switch.
    while (MCG->S & 0x10);

    // Now configure the PLL for 168 Mhz = 8 x 21.  The VCO runs at 336MHz, with
    // a /2 on the output.
    MCG->C6 = 5;                        // *21.
    // FIXME : "OSCINIT 0 bit should be checked"
    // It appears that the MCG block diagram is incorrect.  It shows a /2
    // divider inside the control loop.  It appears to be outside the control
    // loop.  Hence the output clock is half the PLL freq.
    MCG->C5 = 0x60;                     // Enable, Stop-en

    // Wait for lock...
    while (~MCG->S & 0x40);

    // Now select PLLS from PLL not FLL.  Keep *21.
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

    // K66 gotcha: The MPU refuses peripheral accesses by default.  Just turn
    // off the MPU for now...
    MPU->CESR &= ~1;

    // K66 gotcha: The flash controller disables peripheral access by defualt.
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

    usb_reset();

    // DP pullup...
    USB0->OTGCTL = 0x82;                // DPHIGH + OTGEN (automatic).
    USB0->CONTROL = 0x10;               // non-OTG pull-up...

    // Release from suspend, weak pull downs.
    USB0->USBCTRL = 0;

    // Unmask STALL, TOKDONE, RST.  4=SOF
    USB0->INTEN = 0x80 + 9;

    usb_reset();

    // Enable the USB interrupt...
    NVIC->ISER[i_USBFS >> 5] = 1 << (i_USBFS & 31);

    ungetchar(-1);

    monitor();
}


VECTORS_t __attribute__ ((section (".start"), externally_visible)) start = {
    { (void*) 0x1ffffff0, go },
    { [i_USBFS] = usb_interrupt },
};

uint32_t flash_config[4] __attribute__ ((section (".fconfig"), externally_visible));
uint32_t flash_config[4] = { -1, -1, -1, -2 };
