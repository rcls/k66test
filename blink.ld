MEMORY {
  vectors(WX) : ORIGIN = 0, LENGTH = 1024
  fconfig(WX) : ORIGIN = 0x400, LENGTH = 16
  text(WX) : ORIGIN = 0x410, LENGTH = 32K
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
  } > text
}
