### DMA driver programs

### Basic example
`riscv64-unknown-elf-gcc hello.c -o hello.elf`


#### Bare metal with uart driver
`riscv64-unknown-elf-gcc main.c uart.c -o main.elf`


#### User level full driver
`riscv64-unknown-elf-gcc tests.c -o tests.elf`


#### Bare metal fromhost / tohost
`riscv64-unknown-elf-gcc sim_com.c -o sim_com.elf`

