/*
 * Copyright (c) 2021-2022 Andreas Kuster, ETH Zurich and University of Bologna
 * Copyright (c) 2012-2015, The Regents of the University of California (Regents).
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Regents nor the
 *   names of its contributors may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 * OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
 * BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
 * HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Author: Andreas Kuster <kustera@ethz.ch>
 * Description: PMP setup and configuration library
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "encoding.h"

#include "pmp.h"
#include "uart.h"

int granule;

void detect_pmp() {

    // read/write base csr registers
    write_csr(pmpaddr0, 42);
    write_csr(pmpaddr1, 42);
    write_csr(pmpcfg0, 42);

    if (read_csr(pmpcfg0) != 42 || read_csr(pmpaddr0) != 42 || read_csr(pmpaddr1) != 42) {
        print_uart("Pmp: read/write failed\n");
    } else {
        print_uart("Dectect pmp: pmp0 detected\n");
    }
}

void detect_granule() {
    // detect pmp granule size
    write_csr(pmpcfg0, NULL);
    write_csr(pmpaddr0, 0xffffffffffffffffULL);
    uintptr_t ret = read_csr(pmpaddr0);
    int g = 2;
    for (uintptr_t i = 1; i; i <<= 1) {
        if ((ret & i) != 0)
            break;
        g++;
    }
    granule = 1UL << g;

    // print outcome
    print_uart("Pmp granule: ");
    print_uart_int(granule);
    print_uart("\n");
}

uintptr_t read_pmp_locked(uintptr_t addr, uintptr_t size) {

    // read from address in pmp-locked state (i.e. to test in m-mode if pmp work)
    uintptr_t new_mstatus = (read_csr(mstatus) & ~MSTATUS_MPP) | (MSTATUS_MPP & (MSTATUS_MPP >> 1)) | MSTATUS_MPRV;
    uintptr_t value = 0;

    switch (size) {
        case 1:
            asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr));
            asm volatile ("lb %[val], (%1);" : [val]"+&r"(value) : "r"(addr));
            asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr));
            break;
        case 2:
            asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr));
            asm volatile ("lh %[val], (%1);" : [val]"+&r"(value) : "r"(addr));
            asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr));
            break;
        case 4:
            asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr));
            asm volatile ("lw %[val], (%1);" : [val]"+&r"(value) : "r"(addr));
            asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr));
            break;
#if __riscv_xlen >= 64
            case 8:
              asm volatile ("csrrw %0, mstatus, %0;" : "+&r" (new_mstatus) : "r" (addr));
              asm volatile ("ld %[val], (%1);" : [val]"+&r"(value) : "r" (addr));
              asm volatile ("csrw mstatus, %0" : "+&r" (new_mstatus) : "r" (addr));
              break;
#endif
        default:
            __builtin_unreachable();
    }

    return value;
}

void set_pmp_zero() {
    // TODO: not sure why this is done in riscv-tests
    write_csr(pmpaddr0, 0);
    asm volatile ("sfence.vma":: : "memory");
}

pmpcfg_t set_pmp(pmpcfg_t p) {

    // write config
    // note: on XLEN=64 the first 8 pmp configs are in pmpcfg0
    uintptr_t cfg0 = read_csr(pmpcfg0);
    uintptr_t mask = (0xff << (8 * p.slot));
    write_csr(pmpcfg0, cfg0 & ~mask);

    switch (p.slot) {
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

    write_csr(pmpcfg0, ((p.cfg << (8 * p.slot)) & mask) | (cfg0 & ~mask));

    asm volatile ("sfence.vma":: : "memory");
    return p;
}

pmpcfg_t set_pmp_napot(uintptr_t base, uintptr_t range, uintptr_t slot) {
    // set pmp with r/w/x access
    return set_pmp_napot_access(base, range, (PMP_W | PMP_R | PMP_X), slot);
}

pmpcfg_t set_pmp_napot_access(uintptr_t base, uintptr_t range, uintptr_t access, uintptr_t slot) {
    pmpcfg_t p;
    p.cfg = access | (range > granule ? PMP_NAPOT : PMP_NA4);
    p.a0 = (base + (range / 2 - 1)) >> PMP_SHIFT;
    p.slot = slot;
    return set_pmp(p);
}
