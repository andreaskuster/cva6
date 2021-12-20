# start sequence of the bootloader
#
#
#define DRAM_BASE 0x80000000

  .section .text.init
  .option norvc
  .globl _prog_start
_prog_start:
  li sp, 0x81000000
  call main
