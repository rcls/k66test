MEMORY {
  vectors(RX) : ORIGIN = 0, LENGTH = 1024
  fconfig(RX) : ORIGIN = 0x400, LENGTH = 16
  text(RX) : ORIGIN = 0x410, LENGTH = 0xffbf0

  /* 256k with a 64+192 split */
  ramL(WX) : ORIGIN = 0x1fff0000, LENGTH = 64K
  ramH(WX) : ORIGIN = 0x20000000, LENGTH = 192K
}

SECTIONS
{
  .vectors : {
     KEEP(*(.start))
  } > vectors

  .fconfig : {
     KEEP(*(.fconfig))
  } > fconfig

  .text : {
     *(.text*)
     *(SORT_BY_ALIGNMENT(.rodata*))
  } > text

  .bss (NOLOAD) : {
     bss_start = .;
     *(SORT_BY_ALIGNMENT(.bss*))
     bss_end = .;
  } > ramL
}
