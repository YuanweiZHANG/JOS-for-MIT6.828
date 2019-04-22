// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!((err & FEC_WR) && (uvpt[PGNUM(addr)] & PTE_COW))) {
		panic("pgfault: faulting access error, 0x%8x\n", addr);
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if (sys_page_alloc(0, (void *)PFTEMP, PTE_P | PTE_U | PTE_W) < 0) {
		panic("pgfault: page alloc error, va: 0x%08x", PFTEMP);
	}

	memcpy((void *)PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE); // Cautious: addr may not align to PGSIZE

	if (sys_page_map(0, (void *)PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_U | PTE_W) < 0) {
		// Cautious: DO NOT mix up addr and PFTEMP, addr is dst
		panic("pgfault: page map error, srcva: 0x%08x, dstva: 0x%08x", addr, PFTEMP);
	}

	if (sys_page_unmap(0, PFTEMP) < 0) {
		panic("pgfault: page unmap error, va: 0x%08x", PFTEMP);
	}
	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	void *va = (void *)(pn * PGSIZE);
	int perm = uvpt[PGNUM(va)] & 0xFFF & PTE_SYSCALL; // 0xFFF |= PTE_
	// Cautious: PTE_SYSCALL is vital, or sys_page_map gets wrong perm

	if (perm & (PTE_W | PTE_COW)) {
		perm |= PTE_COW;
		perm &= (~PTE_W); // Cautious: in Lab4 duppage sets both PTEs so that the page is not writeable
		if ((r = sys_page_map(0, va, envid, va, perm)) < 0) { // new mapping must be created COW
			return r;
		}
		if ((r = sys_page_map(0, va, 0, va, perm)) < 0) { // our mapping must be marked COW as well
			return r;
		}
	}
	else {
		if ((r = sys_page_map(0, va, envid, va, perm)) < 0) {
			return r;
		}
	} 
	// panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int r;
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();

	if (envid < 0) {
		panic("fork: sys_exofork: %e", envid);
	}
	else if (envid == 0) { // child
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	bool flag = false;
	for (int i = 0; i < NPDENTRIES; ++i) {
		if (uvpd[i] & PTE_P) {
			for (int j = 0; j < NPTENTRIES; ++j) {
				int pn = i * NPTENTRIES + j;
				if (pn == PGNUM(USTACKTOP)) {
					flag = true;
					break;
				}
				if (uvpt[pn] & PTE_P) {
					if ((r = duppage(envid, pn)) < 0) {
						panic("fork: duppage error: %e", r);
					}
				}
			}
			if (flag) {
				break;
			}
		}
	}

	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_W | PTE_U)) < 0) {
		// allocate a fresh page in the child for the exception stack
		return r;
	}

	extern void _pgfault_upcall(void);
	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0) {
		return r;
	}

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
		return r;
	}
	return envid;
	panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
