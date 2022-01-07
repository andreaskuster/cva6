// Copyright (c) 2021 ETH Zurich and University of Bologna
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the "License"); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
// Author: Andreas Kuster <kustera@ethz.ch>
//
// Description: Simple iDMA engine / PMP / IO-PMP testing program (DMA attack)

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


//#define DMA_BASE (0x80000000 + 0x20000000) // dummy device dram address
#define DMA_BASE 0x50000000  // dma

#define DMA_SRC_ADDR      (DMA_BASE + DMA_FRONTEND_SRC_ADDR_REG_OFFSET)
#define DMA_DST_ADDR      (DMA_BASE + DMA_FRONTEND_DST_ADDR_REG_OFFSET)
#define DMA_NUMBYTES_ADDR (DMA_BASE + DMA_FRONTEND_NUM_BYTES_REG_OFFSET)
#define DMA_CONF_ADDR     (DMA_BASE + DMA_FRONTEND_CONF_REG_OFFSET)
#define DMA_STATUS_ADDR   (DMA_BASE + DMA_FRONTEND_STATUS_REG_OFFSET)
#define DMA_NEXTID_ADDR   (DMA_BASE + DMA_FRONTEND_NEXT_ID_REG_OFFSET)
#define DMA_DONE_ADDR     (DMA_BASE + DMA_FRONTEND_DONE_REG_OFFSET)

#define DMA_TRANSFER_SIZE (2*8) //(4 * 1024) // 4KB, i.e. page size

#define DMA_CONF_DECOUPLE 0
#define DMA_CONF_DEBURST 0
#define DMA_CONF_SERIALIZE 0

#define ASSERT(expr, msg)             \
if (!(expr)) {                        \
    print_uart("assertion failed: "); \
    print_uart(msg);                  \
    print_uart("\n");                 \
    return -1;                        \
}

static inline void sleep_loop(){

    for(int i = 0; i < 16*DMA_TRANSFER_SIZE; i++){ 
        asm volatile ("ADDI x0, x1, 0" :  : ); // nop operation
        //asm volatile ("nop" :  : ); // nop operation
    }
}

int main(int argc, char const *argv[]) {

    // asm volatile("li sp, 0x81000000"); // set up stack

    init_uart(50000000, 115200);
    print_uart("Hello CVA6 from iDMA!\n");


    setup_trap();



//   volatile uint64_t* test = (volatile uint64_t*)0x0;
//   uint64_t asfd = *test;
//    print_uart_int(asfd);
    /*
     * Setup relevant configuration registers
     */
    volatile uint64_t* dma_src = (volatile uint64_t*)DMA_SRC_ADDR;
    volatile uint64_t* dma_dst = (volatile uint64_t*)DMA_DST_ADDR;
    volatile uint64_t* dma_num_bytes = (volatile uint64_t*)DMA_NUMBYTES_ADDR;
    volatile uint64_t* dma_conf = (volatile uint64_t*)DMA_CONF_ADDR;
    //volatile uint64_t* dma_status = (volatile uint64_t*)DMA_STATUS_ADDR;
    volatile uint64_t* dma_nextid = (volatile uint64_t*)DMA_NEXTID_ADDR;
    volatile uint64_t* dma_done = (volatile uint64_t*)DMA_DONE_ADDR;



    /*
     * Prepare data
     */
    // allocate DMA_TRANSFER_SIZE bytes
    uint64_t src[DMA_TRANSFER_SIZE / sizeof(uint64_t)];
    //uint64_t guard[512]; // 4KB guard band
    uint64_t dst[DMA_TRANSFER_SIZE / sizeof(uint64_t)];

    // [debug] print stack address
    // print_uart("charptr@");
    // print_uart_addr(&src);
    // print_uart("\n");

    // fill src array & clear dst array
    for(size_t i = 0; i < DMA_TRANSFER_SIZE / sizeof(uint64_t); i++){
        src[i] = 42;
        dst[i] = 0;
    }

    /*
     * PMP: Guard source data
     */

    // pmpcfg_t pmp0 = {
    //   .cfg = ,
    //   .a0 = ,
    //   .a1 = 
    // }

//    detect_pmp();
    detect_granule();
    set_pmp_zero();
/*
    // try protecting non-cached region instead
    // pmpcfg_t pmp0 = set_pmp_range((uintptr_t)UART_BASE, 16);
    pmpcfg_t pmp0 = set_pmp_napot((uintptr_t)UART_BASE, 16);    
    test_one((uintptr_t)UART_BASE, 4);
    
    //write_reg_u8(UART_BASE, 'a');
    //read_reg_u8(UART_BASE);
*/

/*
    // whitelist dst array region
    pmpcfg_t pmp0 = set_pmp_napot((uintptr_t)&dst, DMA_TRANSFER_SIZE);
    //pmpcfg_t pmp0 = set_pmp_range((uintptr_t)&src, DMA_TRANSFER_SIZE);
    test_one((uintptr_t)&dst, 4); // TODO: <- this should work
    
    //test_one((uintptr_t)&src, 4); // TODO: <- access violation
*/

    //set_pmp_napot_access((uintptr_t)&src, DMA_TRANSFER_SIZE, PMP_R | PMP_W | PMP_X);
    //set_pmp_napot_access((uintptr_t)&src, DMA_TRANSFER_SIZE, PMP_NO_ACCESS, 1);
    set_pmp_napot_access((uintptr_t)&src, DMA_TRANSFER_SIZE, (PMP_R | PMP_W | PMP_X), 1); // allow src access for testing purpose
    set_pmp_napot_access((uintptr_t)&dst, DMA_TRANSFER_SIZE, PMP_NO_ACCESS, 2); //
    //test_one((uintptr_t)&dst, 8); // TODO: <- this should work
    //print_uart("reading dst worked :)\n");
    //test_one((uintptr_t)&src, 8); // TODO: <- access violation
    //print_uart("reading src worked :( \n");

    //pmpcfg_t pmp1 = set_pmp_range(&src, 8);//DMA_TRANSFER_SIZE);
    //test_one(&src, 8);// DMA_TRANSFER_SIZE);
    //src[0] = 42;
    
    // for(int i = 0; i < 10; i++){
    //     print_uart("try reading address ");
    //     print_uart_int(&src[i]);
    //     print_uart(" value: ");
    //     print_uart_int(src[i]);
    //     print_uart("\n");
    // }

    /*
     * Test register access
     */
    print_uart("Test register read/write\n");
    
    // *dma_src = 42;
    // int test = *dma_src;
    // //while(1);
    // print_uart("dma_src written\n");
    
    // print_uart("dma_src: ");
    // print_uart_int(*dma_src);
    // print_uart("\n");
    // ASSERT(*dma_src == 42, "");

    // TODO: enable
    // *dma_src = 42;
    // *dma_dst = 42;
    // *dma_num_bytes = 0;
    // *dma_conf = 7;   // 0b111

    // ASSERT(*dma_src == 42, "dma_src");
    // ASSERT(*dma_dst == 42, "dma_dst");
    // ASSERT(*dma_num_bytes == 0, "dma_num_bytes");
    // ASSERT(*dma_conf == 7, "dma_conf");

    /*
     * Test DMA transfer
     */
    print_uart("Initiate dma request\n");

    // TODO: flush cache?

    // setup src to dst memory transfer
    *dma_src = (uint64_t)&src;
    *dma_dst = (uint64_t)&dst;
    *dma_num_bytes = DMA_TRANSFER_SIZE;
    *dma_conf = (DMA_CONF_DECOUPLE << DMA_FRONTEND_CONF_DECOUPLE_BIT) | (DMA_CONF_DEBURST << DMA_FRONTEND_CONF_DEBURST_BIT) | (DMA_CONF_SERIALIZE << DMA_FRONTEND_CONF_SERIALIZE_BIT);

    print_uart("Start transfer\n");

    // launch transfer: read id
    //uint64_t transfer_id  = 0;

    //for(int i = 0;3 > i; i++){
    uint64_t transfer_id = *dma_nextid; // = DMA_TRANSFER_ID;

    // work-around: add delay to free axi bus (axi_node does not allow parallel transactions -> need to upgrade axi xbar)
    // sleep_loop();
    for(int i = 0; i < 16*DMA_TRANSFER_SIZE; i++){ 
        //asm volatile ("ADDI x0, x1, 0" :  : ); // nop operation
        asm volatile ("nop" :  : ); // nop operation
    }


    // poll wait for transfer to finish
    do {
        print_uart("Transfer finished: ");
        print_uart("transfer_id: ");
        print_uart_int(transfer_id);         
        print_uart(" done_id: ");
        print_uart_int(*dma_done);        
        print_uart(" dst[0]: ");
        print_uart_int(dst[0]);
        print_uart("\n");
    } while(*dma_done != transfer_id);

    // TODO: invalidate cache?

    // check result
    for(size_t i = 0; i < DMA_TRANSFER_SIZE / sizeof(uint64_t); i++){

        uintptr_t dst_val = read_pmp_locked((uintptr_t)&(dst[i]), 8); //dst[i];//;
        print_uart("Try reading dst: ");
        print_uart_int(dst_val);
        print_uart("\n");       

        // uintptr_t src_val = read_pmp_locked((uintptr_t)&(src[i]), 8);//dst[i];//;
        // print_uart("try reading src: ");
        // print_uart_int(src_val);
        // print_uart("\n");          

        // test_one((uintptr_t)&dst, 8); // TODO: <- this should work
        // print_uart("reading dst worked :)\n");
        // test_one((uintptr_t)&src, 8); // TODO: <- access violation
        // print_uart("reading src worked :( \n");


        ASSERT(dst_val == 42, "dst");
    }
    print_uart("Transfer successfully validated.\n");

    // // free allocated memory
    // free(src);
    // free(dst);
    
    print_uart("All done, spin-loop.\n");
    while (1){
        // do nothing
    }

    return 0;
}
