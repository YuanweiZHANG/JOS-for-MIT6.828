#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000;

int e1000_attach(struct pci_func *pcif) {
    pci_func_enable(pcif);
    return 0;
}
