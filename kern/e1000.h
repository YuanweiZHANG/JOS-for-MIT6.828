#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VENDOR_ID  0x8086
#define E1000_DEVICE_ID  0x100E

// Cautious: e1000 MMIO address is uint32_t *, so must divided by sizeof(uint32_t)
#define E1000_STATUS   0x00008/sizeof(uint32_t)  /* Device Status - RO */

int e1000_attach(struct pci_func *pcif);
#endif  // SOL >= 6
