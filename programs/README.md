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
git checkout add_dma
git submodule update --init --recursive

# clone iDMA
git clone https://iis-git.ee.ethz.ch/bslk/idma.git
cd idma
git checkout cva6

# verilate design
make verilate
```

Simulate bare-metal application using verilator
```bash
# run simulation
work-ver/Variane_testharness program/dma_attack.elf
```

Example output:
```
Hello CVA6 from iDMA!
Source array      @0x0000000080FFFF88
Destination array @0x0000000080FFFB78
Dectect pmp: pmp0 detected
Pmp granule: 00000008
PMP 1 setup to lock src array.
PMP 2 setup to give full r/w/x access to dst array
Test register read/write
Initiate dma request
Start transfer
Transfer finished: transfer_id: 00000002 done_id: 00000002 dst[0]: 0000002A
Try reading dst: 0x0000002A
Try reading dst: 0x0000002A
Transfer successfully validated.
All done, spin-loop.

```
