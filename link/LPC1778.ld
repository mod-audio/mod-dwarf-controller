
/* Define each memory region */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x0, LENGTH = 0x80000           /* 512K bytes */
  SRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 0x10000    /* 64K bytes */
  SRAM0 (rwx) : ORIGIN = 0x20000000, LENGTH = 0x8000    /* 32K bytes */
}

/* Define a symbol for the top of each memory region */
__top_FLASH = 0x0 + 0x80000;
__top_SRAM = 0x10000000 + 0x10000;
__top_SRAM0 = 0x20000000 + 0x8000;