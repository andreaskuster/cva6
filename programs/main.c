/*
 * Copyright (c) 2021-2022 ETH Zurich and University of Bologna
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
 * Description: Simple iDMA engine / PMP / IO-PMP testing program (DMA attack)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "uart.h"
#include "pmp.h"
#include "trap.h"
#include "encoding.h"
#include "cva6_idma.h"
#include "iopmp.h"
#include "io_pmp.h"


#define IOPMP_BASE        0x50010000 // io-pmp
#define IOPMP_ADDR0       (IOPMP_BASE + IO_PMP_PMP_ADDR_0_REG_OFFSET)
#define IOPMP_CFG0        (IOPMP_BASE + IO_PMP_PMP_CFG_0_REG_OFFSET)


#define DMA_BASE          0x50000000  // dma
#define DMA_SRC_ADDR      (DMA_BASE + DMA_FRONTEND_SRC_ADDR_REG_OFFSET)
#define DMA_DST_ADDR      (DMA_BASE + DMA_FRONTEND_DST_ADDR_REG_OFFSET)
#define DMA_NUMBYTES_ADDR (DMA_BASE + DMA_FRONTEND_NUM_BYTES_REG_OFFSET)
#define DMA_CONF_ADDR     (DMA_BASE + DMA_FRONTEND_CONF_REG_OFFSET)
#define DMA_STATUS_ADDR   (DMA_BASE + DMA_FRONTEND_STATUS_REG_OFFSET)
#define DMA_NEXTID_ADDR   (DMA_BASE + DMA_FRONTEND_NEXT_ID_REG_OFFSET)
#define DMA_DONE_ADDR     (DMA_BASE + DMA_FRONTEND_DONE_REG_OFFSET)

#define DMA_TRANSFER_SIZE (2 * sizeof(uint64_t)) //(4 * 1024) // 4KB, i.e. page size

#define DMA_CONF_DECOUPLE 0
#define DMA_CONF_DEBURST 0
#define DMA_CONF_SERIALIZE 0


#define TEST_TRAP 0
#define TEST_PMP_SRC 0
#define VERBOSE 1

#define ASSERT(expr, msg)             \
if (!(expr)) {                        \
    print_uart("assertion failed: "); \
    print_uart(msg);                  \
    print_uart("\n");                 \
    print_uart("Spin-loop.\n");       \
    while (1) {};                     \
}

static inline void sleep_loop() {
    for (int i = 0; i < 16 * DMA_TRANSFER_SIZE; i++) {
        asm volatile ("nop" :  : ); // nop operation
    }
}


int main(int argc, char const *argv[]) {

    /*
     * Setup uart
     */
    init_uart(50000000, 115200);
    print_uart("Hello CVA6 from iDMA!\n");

    /*
     * Setup trap
     */
    setup_trap();

    if (TEST_TRAP) { // cause null ptr exception
        volatile uintptr_t *null_ptr = (volatile uintptr_t *) 0x0;
        print_uart_int(*null_ptr);
    }

    /*
     * Setup relevant configuration registers
     */
    volatile uint64_t *dma_src = (volatile uint64_t *) DMA_SRC_ADDR;
    volatile uint64_t *dma_dst = (volatile uint64_t *) DMA_DST_ADDR;
    volatile uint64_t *dma_num_bytes = (volatile uint64_t *) DMA_NUMBYTES_ADDR;
    volatile uint64_t *dma_conf = (volatile uint64_t *) DMA_CONF_ADDR;
    // volatile uint64_t* dma_status = (volatile uint64_t*)DMA_STATUS_ADDR; // not used in current implementation
    volatile uint64_t *dma_nextid = (volatile uint64_t *) DMA_NEXTID_ADDR;
    volatile uint64_t *dma_done = (volatile uint64_t *) DMA_DONE_ADDR;

    /*
     * Prepare data
     */
    // allocate source array
    uint64_t src[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
    if (VERBOSE) {
        // print array stack address
        print_uart("Source array @ 0x");
        print_uart_addr((uint64_t) & src);
        print_uart("\n");
    }

    // 4KB guard
    char __attribute__((unused)) guard[4096] __attribute__ ((aligned (4096)));

    // allocate destination array
    uint64_t dst[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
    if (VERBOSE) {
        // print array stack address
        print_uart("Destination array @ 0x");
        print_uart_addr((uint64_t) & dst);
        print_uart("\n");
    }

    // fill src array & clear dst array
    for (size_t i = 0; i < DMA_TRANSFER_SIZE / sizeof(uint64_t); i++) {
        src[i] = 42;
        dst[i] = 0;
    }

    // TODO: flush cache?

    /*
     * PMP: Guard source data
     */
    detect_pmp();
    detect_granule();
    set_pmp_zero();

    // set_pmp_allow_all(0); // for debugging, should allow access to the complete address space

    // block access to source array
    set_pmp_napot_access((uintptr_t) &src, DMA_TRANSFER_SIZE, PMP_NO_ACCESS, 0); // does not have to be done explicitly..but can
    // however, allow access to destination array
    set_pmp_napot_access(((uintptr_t) &dst)-DMA_TRANSFER_SIZE, 2*DMA_TRANSFER_SIZE, (PMP_R | PMP_W | PMP_X), 1);

    /*
     * Setup IO-PMP: allow
     */
    detect_iopmp();
    detect_iopmp_granule();
    set_iopmp_allow_all(0); // for debugging, should allow access to the complete address space
    // set_iopmp_napot_access(((uintptr_t) &src), 4096, (PMP_R | PMP_W | PMP_X), 0);
    // set_iopmp_napot_access(((uintptr_t) &dst), 4096, (PMP_R | PMP_W | PMP_X), 1);

    volatile uint64_t *iopmp_addr0 = (volatile uint64_t *) IOPMP_ADDR0;
    volatile uint64_t *iopmp_cfg0  = (volatile uint64_t *) IOPMP_CFG0;
    print_uart("iopmp_addr0: ");
    print_uart_addr(*iopmp_addr0);
    print_uart(" iopmp_cfg0: ");
    print_uart_addr(*iopmp_cfg0);
    print_uart("\n");


    /*
     * Test register access
     */
    print_uart("Test register read/write\n");

    // test register read/write
    *dma_src = 42;
    *dma_dst = 42;
    *dma_num_bytes = 0;
    *dma_conf = 7;   // 0b111

    ASSERT(*dma_src == 42, "dma_src");
    ASSERT(*dma_dst == 42, "dma_dst");
    ASSERT(*dma_num_bytes == 0, "dma_num_bytes");
    ASSERT(*dma_conf == 7, "dma_conf");

    /*
     * Test DMA transfer
     */
    print_uart("Initiate dma request\n");

    // setup src to dst memory transfer
    *dma_src = (uint64_t) & src;
    *dma_dst = (uint64_t) & dst;
    *dma_num_bytes = DMA_TRANSFER_SIZE;
    *dma_conf = (DMA_CONF_DECOUPLE  << DMA_FRONTEND_CONF_DECOUPLE_BIT) |
                (DMA_CONF_DEBURST   << DMA_FRONTEND_CONF_DEBURST_BIT) |
                (DMA_CONF_SERIALIZE << DMA_FRONTEND_CONF_SERIALIZE_BIT);

    print_uart("Start transfer\n");

    // launch transfer: read id
    uint64_t transfer_id = *dma_nextid;

    // poll wait for transfer to finish
    do {
        print_uart("transfer_id: ");
        print_uart_int(transfer_id);
        print_uart(" done_id: ");
        print_uart_int(*dma_done);
        print_uart(" dst[0]: ");
        print_uart_int(dst[0]);
        print_uart("\n");
    } while (*dma_done != transfer_id);

    print_uart("Transfer finished\n");

    // TODO: flush cache?

    // check result
    for (size_t i = 0; i < DMA_TRANSFER_SIZE / sizeof(uint64_t); i++) {

        uintptr_t dst_val = read_pmp_locked((uintptr_t) & (dst[i]), 8);
        print_uart("Try reading dst: 0x");
        print_uart_addr(dst_val);
        print_uart("\n");

        ASSERT(dst_val == 42, "dst");

        if (TEST_PMP_SRC) {
            uintptr_t src_val = read_pmp_locked((uintptr_t) & (src[i]), 8);
            print_uart("Try reading src: 0x");
            print_uart_int(src_val);
            print_uart("\n");
        }

    }
    print_uart("Transfer successfully validated.\n");


    /*
     * Reset dst array
     */
    print_uart("Reset destination array.\n");
    for (size_t i = 0; i < DMA_TRANSFER_SIZE / sizeof(uint64_t); i++) {
        dst[i] = 0;
    }

    /*
     * Setup IO-PMP: disallow
     */
    print_uart("IO-PMP: Lock src array.\n");
    set_iopmp_napot_access(((uintptr_t) &src), 4096, PMP_NO_ACCESS, 0);
    print_uart("iopmp_addr0: ");
    print_uart_addr(*iopmp_addr0);
    print_uart(" iopmp_cfg0: ");
    print_uart_addr(*iopmp_cfg0);
    print_uart("\n");


    /*
     * Test DMA transfer
     */
    print_uart("Initiate dma request\n");

    // setup src to dst memory transfer
    *dma_src = (uint64_t) & src;
    *dma_dst = (uint64_t) & dst;
    *dma_num_bytes = DMA_TRANSFER_SIZE;
    *dma_conf = (DMA_CONF_DECOUPLE  << DMA_FRONTEND_CONF_DECOUPLE_BIT) |
                (DMA_CONF_DEBURST   << DMA_FRONTEND_CONF_DEBURST_BIT) |
                (DMA_CONF_SERIALIZE << DMA_FRONTEND_CONF_SERIALIZE_BIT);

    print_uart("Start transfer\n");

    // launch transfer: read id
    transfer_id = *dma_nextid;

    // poll wait for transfer to finish
    do {
        print_uart("transfer_id: ");
        print_uart_int(transfer_id);
        print_uart(" done_id: ");
        print_uart_int(*dma_done);
        print_uart(" dst[0]: ");
        print_uart_int(dst[0]);
        print_uart("\n");
    } while (*dma_done != transfer_id);

    print_uart("Transfer finished\n");

    // TODO: flush cache?

    // check result
    for (size_t i = 0; i < DMA_TRANSFER_SIZE / sizeof(uint64_t); i++) {

        uintptr_t dst_val = read_pmp_locked((uintptr_t) & (dst[i]), 8);
        print_uart("Try reading dst: 0x");
        print_uart_addr(dst_val);
        print_uart("\n");

        ASSERT(dst_val == 42, "dst");

        if (TEST_PMP_SRC) {
            uintptr_t src_val = read_pmp_locked((uintptr_t) & (src[i]), 8);
            print_uart("Try reading src: 0x");
            print_uart_int(src_val);
            print_uart("\n");
        }

    }
    print_uart("Transfer successfully validated.\n");




    print_uart("All done, spin-loop.\n");
    while (1) {
        // do nothing
    }

    return 0;
}
