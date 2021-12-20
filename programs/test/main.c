#include "uart.h"

int main(){
    // asm volatile("li sp, 0x81000000"); // set up stack

    init_uart(50000000, 115200);
    print_uart("Hello World!\r\n");

    print_uart("this is a test\r\n");


    while (1){
        // do nothing
    }
}

void handle_trap(void){
    print_uart("trap\r\n");
}
