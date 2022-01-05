// See LICENSE for license details.

// Test of PMP functionality.

#define INLINE inline __attribute__((always_inline))


typedef struct {
  uintptr_t cfg;
  uintptr_t a0;
  uintptr_t slot;
  //uintptr_t a1;
} pmpcfg_t;

void detect_pmp();
void detect_granule();
uintptr_t read_pmp_locked(uintptr_t addr, uintptr_t size);
pmpcfg_t set_pmp(pmpcfg_t p);
void set_pmp_zero();
pmpcfg_t set_pmp_napot(uintptr_t base, uintptr_t range, uintptr_t slot);
pmpcfg_t set_pmp_napot_access(uintptr_t base, uintptr_t range, uintptr_t access, uintptr_t slot);
pmpcfg_t set_pmp_range(uintptr_t base, uintptr_t range);
