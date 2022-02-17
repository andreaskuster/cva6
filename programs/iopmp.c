/*
 * Copyright 2022 ETH Zurich and University of Bologna.
 * Copyright and related rights are licensed under the Solderpad Hardware
 * License, Version 0.51 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
 * or agreed to in writing, software, hardware and materials distributed under
 * this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * Author: Andreas Kuster <kustera@ethz.ch>
 * Description: IO-PMP setup and configuration library
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "encoding.h"

#include "iopmp.h"
#include "uart.h"

#define IOPMP_BASE      0x50010000
#define IOPMP_ADDR_BASE (IOPMP_BASE + 0x0)
#define IOPMP_CFG_BASE  (IOPMP_BASE + 0x80)
#define IOPMP_NUM_PMP   16

int ipmp_granule;

void detect_iopmp() {

    // read/write base registers
    volatile uintptr_t *pmpcfg0 = (volatile uintptr_t *) IOPMP_CFG_BASE;
    volatile uintptr_t *pmpaddr0 = (volatile uintptr_t *) IOPMP_ADDR_BASE;

    // read old value
    uintptr_t old_cfg = *pmpcfg0;
    uintptr_t old_addr = *pmpaddr0;

    // set new value
    *pmpcfg0 = 42;
    *pmpaddr0 = 42;

    // check if read-back value matches
    if (*pmpcfg0 != 42 || *pmpaddr0 != 42) {
        print_uart("IO-PMP0: read/write failed\n");
    } else {
        print_uart("IO-PMP0: detected\n");
    }

    // restore previous value
    *pmpcfg0 = old_cfg;
    *pmpaddr0 = old_addr;
}

void detect_iopmp_granule() {

    // detect pmp granule size according to the RISC-V PMP specs
    volatile uintptr_t *pmpcfg0 = (volatile uintptr_t *) IOPMP_CFG_BASE;
    volatile uintptr_t *pmpaddr0 = (volatile uintptr_t *) IOPMP_ADDR_BASE;

    //  write 0 to config & 0xff..ff to address
    *pmpcfg0 = 0x0ULL;
    *pmpaddr0 = 0xffffffffffffffffULL;

    // read address and extract num_zeros on lsb
    uintptr_t ret = *pmpaddr0;
    int g = 2;
    for (uintptr_t i = 1; i; i <<= 1) {
        if ((ret & i) != 0)
            break;
        g++;
    }
    ipmp_granule = 1UL << g;

    // print result
    print_uart("IO-PMP granularity: ");
    print_uart_int(ipmp_granule);
    print_uart("\n");
}

iopmpcfg_t set_iopmp(iopmpcfg_t p) {

    if(p.slot < 0 || p.slot >= 16){
        print_uart("PMP invalid slot!\n");
        return p;
    }
    // POST: 0 <= p.slot < 16

    volatile uintptr_t *pmpcfg = (volatile uintptr_t *) (IOPMP_CFG_BASE + ((p.slot < 8) ? 0 : 8));

    uintptr_t mask, shift;
    if(p.slot >= 8){
        shift = p.slot-8;
    } else {
        shift = p.slot;
    }
    mask = (0xff << (8 * shift));

    *pmpcfg = *pmpcfg & ~mask;

    volatile uintptr_t *pmpaddr = (volatile uintptr_t *) (IOPMP_ADDR_BASE + (8 * p.slot));
    *pmpaddr = p.a0;

    *pmpcfg =  ((p.cfg << (8 * shift)) & mask) | (*pmpcfg & ~mask);

    return p;
}

iopmpcfg_t set_iopmp_napot(uintptr_t base, uintptr_t range, uintptr_t slot) {
    // set pmp with r/w/x access
    return set_iopmp_napot_access(base, range, (PMP_W | PMP_R | PMP_X), slot);
}

iopmpcfg_t set_iopmp_napot_access(uintptr_t base, uintptr_t range, uintptr_t access, uintptr_t slot) {
    iopmpcfg_t p;
    p.cfg = access | (range > ipmp_granule ? PMP_NAPOT : PMP_NA4);
    p.a0 = (base + (range / 2 - 1)) >> PMP_SHIFT;
    p.slot = slot;
    return set_iopmp(p);
}

iopmpcfg_t set_iopmp_allow_all(uintptr_t slot) {
    iopmpcfg_t p;
    p.cfg = (PMP_W | PMP_R | PMP_X) | PMP_NAPOT;
    p.a0 = 0xFFFFFFFFFFFFFFFF;
    p.slot = slot;
    return set_iopmp(p);
}
