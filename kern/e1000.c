#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000;
struct e1000_tx_desc tx_descs[NTXDESC];

int e1000_attach(struct pci_func *pcif) {
    pci_func_enable(pcif);
    e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("Exercise 4: status = 0x%x\n", e1000[E1000_STATUS]);
    cprintf("           target is 0x80080783\n");
    e1000_transmit_init();
    return 0;
}

void e1000_transmit_init() {
    e1000[E1000_TDBAL] = PADDR(tx_descs);
    e1000[E1000_TDBAH] = 0;
    e1000[E1000_TDLEN] = sizeof(tx_descs);
    e1000[E1000_TDH] = e1000[E1000_TDT] = 0;
    e1000[E1000_TCTL] |= E1000_TCTL_EN;
    e1000[E1000_TCTL] |= E1000_TCTL_PSP;
    e1000[E1000_TCTL_CT] = E1000_TCTL_CT_INIT;
    e1000[E1000_TCTL_COLD] = E1000_TCTL_COLD_INIT;
    e1000[E1000_TIPG] = E1000_TIPG_INIT;
}