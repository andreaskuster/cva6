// See LICENSE for license details.

// Test of PMP functionality.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "encoding.h"

#include "pmp.h"
#include "uart.h"

int granule;


uintptr_t handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32])
{
  if (cause == CAUSE_ILLEGAL_INSTRUCTION){
    print_uart("Trap CAUSE_ILLEGAL_INSTRUCTION\n"); // no PMP support
  }

  if (cause == CAUSE_LOAD_ACCESS) {
    print_uart("Trap CAUSE_LOAD_ACCESS\n");
  } else {    
    print_uart("Trap OTHER \n");
  }
  return 0;
}

void detect_pmp(){

  write_csr(pmpaddr0, 42);
  write_csr(pmpaddr1, 42);
  write_csr(pmpcfg0, 42);

  if(read_csr(pmpcfg0) != 42 || read_csr(pmpaddr0) != 42 || read_csr(pmpaddr1) != 42){
      print_uart("Pmp: read/write failed\n");
  } else {
      print_uart("Dectect pmp: pmp0 detected\n");
  }
}

void detect_granule()
{
  write_csr(pmpcfg0, NULL);
  write_csr(pmpaddr0, 0xffffffffffffffffULL);
  uintptr_t ret = read_csr(pmpaddr0);
  int g = 2;
  for(uintptr_t i = 1; i; i<<=1) {
    if((ret & i) != 0) 
      break;
    g++;
  }
  granule = 1UL << g;

  print_uart("Pmp granule: ");
  print_uart_int(granule);
  print_uart("\n");  
}

uintptr_t read_pmp_locked(uintptr_t addr, uintptr_t size)
{
  uintptr_t new_mstatus = (read_csr(mstatus) & ~MSTATUS_MPP) | (MSTATUS_MPP & (MSTATUS_MPP >> 1)) | MSTATUS_MPRV;
  uintptr_t value = 0;

  switch (size) {
    case 1:
      asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr)); 
      asm volatile ("lb %[val], (%1);" : [val]"+&r"(value) : "r" (addr)); 
      asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); 
    break;
    case 2: 
      asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr)); 
      asm volatile ("lh %[val], (%1);" : [val]"+&r"(value) : "r" (addr)); 
      asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); 
    break;
    case 4:
      asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr)); 
      asm volatile ("lw %[val], (%1);" : [val]"+&r"(value) : "r" (addr)); 
      asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); 
    break;
#if __riscv_xlen >= 64
    // case 8: asm volatile ("csrrw %0, mstatus, %0; ld x0, (%1); csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); break;
    case 8:
      asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr)); 
      asm volatile ("ld %[val], (%1);" : [val]"+&r"(value) : "r" (addr)); 
      asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); 
      break;
#endif
    default: __builtin_unreachable();
  }

  return value;
}

// void test_one(uintptr_t addr, uintptr_t size)
// {
//   uintptr_t new_mstatus = (read_csr(mstatus) & ~MSTATUS_MPP) | (MSTATUS_MPP & (MSTATUS_MPP >> 1)) | MSTATUS_MPRV;
//   uintptr_t value = 0;

//   switch (size) {
//     case 1: asm volatile ("csrrw %0, mstatus, %0; lb x0, (%1); csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); break;
//     case 2: asm volatile ("csrrw %0, mstatus, %0; lh x0, (%1); csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); break;
//     case 4: asm volatile ("csrrw %0, mstatus, %0; lw x0, (%1); csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); break;
// #if __riscv_xlen >= 64
//     // case 8: asm volatile ("csrrw %0, mstatus, %0; ld x0, (%1); csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); break;
//     case 8:
//       asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr)); 
//       asm volatile ("ld %[val], (%1);" : [val]"+&r"(value) : "r" (addr)); 
//       asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr)); 
//       print_uart("value: ");
//       print_uart_int(value);
//       print_uart("\n"); 
//       break;
// #endif
//     default: __builtin_unreachable();
//   }
// }


// pmpcfg_t set_pmp(pmpcfg_t p)
// {
//   uintptr_t cfg0 = read_csr(pmpcfg0);
//   write_csr(pmpcfg0, cfg0 & ~0xff00);
//   write_csr(pmpaddr0, p.a0);
//   write_csr(pmpaddr1, p.a1);
//   write_csr(pmpcfg0, ((p.cfg << 8) & 0xff00) | (cfg0 & ~0xff00));
//   asm volatile ("sfence.vma" ::: "memory");
//   return p;
// }

void set_pmp_zero(){
  // TODO: not sure why thi
  write_csr(pmpaddr0, 0);
  asm volatile ("sfence.vma" ::: "memory");
}

pmpcfg_t set_pmp(pmpcfg_t p)
{

  uintptr_t cfg0 = read_csr(pmpcfg0);

  uintptr_t mask = (0xff << (8*p.slot));

  write_csr(pmpcfg0, cfg0 & ~mask);


  switch(p.slot){
    case 0:
      write_csr(pmpaddr0, p.a0);
      break;
    case 1:
      write_csr(pmpaddr1, p.a0);
      break;
    case 2:
      write_csr(pmpaddr2, p.a0);
      break;
    case 3:
      write_csr(pmpaddr3, p.a0);
      break;
    case 4:
      write_csr(pmpaddr4, p.a0);
      break;
    case 5:
      write_csr(pmpaddr5, p.a0);
      break;
    case 6:
      write_csr(pmpaddr6, p.a0);
      break;
    case 7:
      write_csr(pmpaddr7, p.a0);
      break;
    default:
      print_uart("PMP invalid slot!\n"); // TODO: extend to 8 (for 8..15 we need pmpcfg1 !!)
      break;
  }
  
  
  write_csr(pmpcfg0, ((p.cfg << (8*p.slot)) & mask) | (cfg0 & ~mask));

  asm volatile ("sfence.vma" ::: "memory");
  return p;
}

// pmpcfg_t set_pmp_range(uintptr_t base, uintptr_t range)
// {
//   pmpcfg_t p;
//   p.cfg = PMP_TOR | PMP_R;
//   p.a0 = base >> PMP_SHIFT;
//   p.a1 = (base + range) >> PMP_SHIFT;
//   return set_pmp(p);
// }

pmpcfg_t set_pmp_napot(uintptr_t base, uintptr_t range, uintptr_t slot){
  return set_pmp_napot_access(base, range, (PMP_W | PMP_R | PMP_X), slot);
}


pmpcfg_t set_pmp_napot_access(uintptr_t base, uintptr_t range, uintptr_t access, uintptr_t slot){
  pmpcfg_t p;
  p.cfg = access | (range > granule ? PMP_NAPOT : PMP_NA4);
  p.a0 = (base + (range / 2 - 1)) >> PMP_SHIFT;
  p.slot = slot;
  return set_pmp(p);
}