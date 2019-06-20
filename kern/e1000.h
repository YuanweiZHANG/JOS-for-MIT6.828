#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VENDOR_ID  0x8086
#define E1000_DEVICE_ID  0x100E

// Cautious: e1000 MMIO address is uint32_t *, so must divided by sizeof(uint32_t)
#define E1000_STATUS   0x00008/sizeof(uint32_t)  /* Device Status - RO */
#define E1000_TCTL     0x00400/sizeof(uint32_t)  /* TX Control - RW */
#define E1000_TIPG     0x00410/sizeof(uint32_t)  /* TX Inter-packet gap -RW */
#define E1000_TDBAL    0x03800/sizeof(uint32_t)  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804/sizeof(uint32_t)  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808/sizeof(uint32_t)  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810/sizeof(uint32_t)  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818/sizeof(uint32_t)  /* TX Descripotr Tail - RW */

// Transmit Control
#define E1000_TCTL_RST      0x00000001    /* software reset */
#define E1000_TCTL_EN       0x00000002    /* enable tx */
#define E1000_TCTL_BCE      0x00000004    /* busy check enable */
#define E1000_TCTL_PSP      0x00000008    /* pad short packets */
#define E1000_TCTL_CT       0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD     0x003ff000    /* collision distance */
#define E1000_TCTL_SWXOFF   0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE      0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC     0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU     0x02000000    /* No Re-transmit on underrun */
#define E1000_TCTL_MULR     0x10000000    /* Multiple request support */

#define E1000_TCTL_CT_INIT       0x10
#define E1000_TCTL_COLD_INIT     0x40
#define E1000_TIPG_INIT          0x60200a      // IPGT = 10, IPGR2 = 6, IPGR1 = 2/3*IPGR2 = 4
// TIPG structure
// -----------------------------------------------------
// | Reserved |    IPGR2    |    IPGR1    |    IPGT    |
// -----------------------------------------------------
// /   31-30  /    29-20    /    19-10    /     9-0    /

// Transmit Descriptor
#define NTXDESC 64
struct e1000_tx_desc {
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
} __attribute__((packed));

int e1000_attach(struct pci_func *pcif);
void e1000_transmit_init();
#endif  // SOL >= 6
