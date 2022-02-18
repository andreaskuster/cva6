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
