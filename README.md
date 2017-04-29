K66 Test Code
=============

Runs on a teensy 3.6 board.

There are a few holes in the K66 documentation from freescale / NXP.
The USBFS documentation in particular is crap, and their example code
is an disasterous mess of poorly thought, incredibly verbose, layering.

Processor start-up
------------------

There are a few gotchas.  They are actually in the documentation if
you know where to look, which is somewhere other than what you are
reading when you want to get stuff done.

* Don't forget about the config bits in flash at address 0x400.  They need
  to be set right.  The documentation is messy but 15 0xff bytes followed by
  an 0xfe byte will get you started.

* The WDOG defaults to enabled and by default leaves you in a continuous
  reboot cycle.  Disable it if you don't want it.

* You need to set SMC_PMPROT to let you set up the run modes you
  want. I'm not sure why they are not allowed by default...

* Set HSRUN if you want to use high clock rates.  Reset HSRUN if you want to
  program flash.  The docs don't actually say what it does, my guess is that
  it changes internal voltage regulators.

* The divide-by-two on the PLL output is poorly documented, and the
  MCG block diagram is incorrect.  The PLL VCO frequency is (input
  frequency / PRDIV * VDIV) and the PLL output is half that.

* If you're using high clock rates, make sure you set the flash clock
  divider SIM_CLKDIV1.

* As usual for Cortex-M SoCs, all peripheral clocks are disabled by
  default.  Enable them in the SIM registers.

* The memory protection unit (MPU) disables peripheral accesses by
  default.  Disable the MPU or reenable as needed.

* Ditto the flash memory controller (FMC).  God knows why they thought
  flash needs its own (non-standard) MPU.  (My guess is some
  corpomoron customer code was so full of security holes that you
  could unexpectedly dump out their supa-sekrit IP from flash and
  point fingers and laugh at said corpomoron, triggering
  self-important and hubristic lack of sense of humour).

USBHS
-----

The USBHS documentation is lacking, esp. for device mode.  If you
know the USB protocol, then the hardware is actually quite sensible.

* You need to manage the data-toggle yourself, which is separate from the
  BDT odd/even.  (Although for a bulk endpoint you can keep them in synch).

* Set-up OUT transfers always use DATA0 (for the first DATA).

* Set-up IN replies always start with DATA1.  Always send an IN in
  response to a set-up (unless you STALL), zero-length of there is
  no data.  Unlike some other USB device implementations, no need to
  explicity handle a zero-length OUT acknowledgement.

* Data toggles reset on a set-up set-config.

* Is the odd/even reset bit self-clearing?
