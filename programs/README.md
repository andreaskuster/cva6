### DMA driver programs
This driver demonstrates the usage of the iDMA engine from the CVA6 application-class RISCV CPU, through the AXI interface.

### Build process

Compile driver:
```bash
cd programs
make
```

Checkout CVA6 and iDMA repo
```bash
# clone cva6 add_dma branch
git clone https://github.com/andreaskuster/cva6.git
cd cva6
git checkout add_dma_upgrade
git submodule update --init --recursive

# clone iDMA
git clone https://iis-git.ee.ethz.ch/bslk/idma.git
cd idma
git checkout cva6

# clone io-pmp
git clone https://github.com/andreaskuster/axi-io-pmp.git
```

Simulate bare-metal application using verilator
```bash
# verilate design
make verilate

# run simulation
work-ver/Variane_testharness programs/dma_attack.elf
```

Simulate using Questasim
```bash
# run simulation
make sim elf-bin=programs/dma_attack.elf batch-mode=1

# preloading memory should be faster
make sim preload=programs/dma_attack.elf batch-mode=1
```


Example output:
```
# Hello CVA6 from iDMA!
# Source array @ 0x0000000080FFE000
# Destination array @ 0x0000000080FFD000
# Detect PMP: PMP0 detected
# PMP granularity: 00000008
# IO-PMP0: detected
# IO-PMP granularity: 00001000
# iopmp_addr0: 003FFFFFFFFFFFFF iopmp_cfg0: 000000000000001F
# Test register read/write
# Initiate dma request
# Start transfer
# transfer_id: 00000002 done_id: 00000002 dst[0]: 0000002A
# Transfer finished
# Try reading dst: 0x000000000000002A
# Try reading dst: 0x000000000000002A
# Transfer successfully validated.
# Reset destination array.
# IO-PMP: Lock src array.
# iopmp_addr0: 00000000203FF9FF iopmp_cfg0: 0000000000000010
# Initiate dma request
# Start transfer
# transfer_id: 00000003 done_id: 00000003 dst[0]: 00000000
# Transfer finished
# Try reading dst: 0x0000000000000000
# assertion failed: dst
# Spin-loop.
```
