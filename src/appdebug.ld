OUTPUT_FORMAT(elf64-x86-64);
OUTPUT_ARCH(i386:x86-64);
ENTRY(_entry)
STARTUP(lib/crt0.o)

SECTIONS
{
  . = 0x40000000;
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
}
