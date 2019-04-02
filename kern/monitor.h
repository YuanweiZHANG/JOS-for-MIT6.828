#ifndef JOS_KERN_MONITOR_H
#define JOS_KERN_MONITOR_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

struct Trapframe;

// Activate the kernel monitor,
// optionally providing a trap frame indicating the current state
// (NULL if none).
void monitor(struct Trapframe *tf);

// Functions implementing monitor commands.
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
int mon_showmappings(int argc, char **argv, struct Trapframe *tf);
int mon_perm(int argc, char **argv, struct Trapframe *tf);
int mon_memory(int argc, char **argv, struct Trapframe *tf);
int mon_si(int argc, char **argv, struct Trapframe *tf);
int mon_c(int argc, char **argv, struct Trapframe *tf);

// Functions implementing showmappings.
void showmappings_help();
void showmappings_vaddr(uintptr_t start, uintptr_t end);
void showmappings_permission(uint32_t *pte_pr);

// Functions implementing perm
void perm_help();
int perm_get(char *argv);
void perm_set(uint32_t *pte_pr, int perm);

// Functions implementing memory
void memory_help();
void memory_virtual(uintptr_t va, uint32_t byte);
void memory_physical(physaddr_t pa, uint32_t byte);

#endif	// !JOS_KERN_MONITOR_H
