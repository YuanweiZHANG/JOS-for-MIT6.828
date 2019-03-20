// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/tcolor.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display backtrace information of stack", mon_backtrace },
	{ "showmappings", "Display pa va mapping, type -h for help", mon_showmappings },
	{ "perm", "Set mapping permisstion, type -h for help", mon_perm},
	{ "memory", "Display va or pa memory content, type -h for help", mon_memory},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	cprintf("Stack backtrace:\n");
	uint32_t *ebp = (uint32_t *)read_ebp();
	while (ebp) {
		cprintf("  ebp %08x  eip %08x  args", ebp, ebp[1]);
		for (int i = 2; i < 7; ++i) {
			cprintf(" %08x", ebp[i]);
		}
		cprintf("\n");
		struct Eipdebuginfo info;
		int success = debuginfo_eip(ebp[1], &info);
		cprintf("         %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, ebp[1] - info.eip_fn_addr);
		ebp = (uint32_t *) (*ebp);
	}

	return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf) 
{
	// check argc
	if (argc < 2) {
		cprintf("Too little arguments in showmappings command\n\n");
		showmappings_help();
		return 0;
	}
	
	if (strcmp(argv[1], "-h") == 0) {
		showmappings_help();
	}
	else if (strcmp(argv[1], "-a") == 0) {
		// show virtual address va's mapping information
		if (argc != 4) {
			cprintf("Wrong arguments in showmappings command\n\n");
			showmappings_help();
			return 0;
		}

		uintptr_t start = ROUNDDOWN((uintptr_t)strtol(argv[2], NULL, 16), PGSIZE);
		uintptr_t end = ROUNDUP((uintptr_t)strtol(argv[3], NULL, 16), PGSIZE);

		showmappings_vaddr(start, end);
	}
	else {
		showmappings_help();
	}
	return 0;
}

int mon_perm(int argc, char **argv, struct Trapframe *tf) {
	// check argc
	if (argc < 2) {
		cprintf("Too little arguments in perm command\n\n");
		perm_help();
		return 0;
	}

	int perm = 0;  // perm are set by the last instruction of -s/-c

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-h") == 0) {
			perm_help();
			return 0;
		}
		else if (strcmp(argv[i], "-s") == 0) {
			// set permission
			i++;
			perm = perm_get(argv[i]);
			continue;
		}
		else if (strcmp(argv[i], "-c") == 0) {
			// clean permission to -rw
			perm = PTE_W;
			continue;
		}
		else if (strcmp(argv[i], "-a") == 0) {
			// set or clean permission on virtual address va
			for (i = i + 1; i < argc; ++i) {
				if (argv[i][0] == '-') {
					break;
				}
				uintptr_t va = (uintptr_t)strtol(argv[i], NULL, 16);
				pte_t *pte_pr;
				if (page_lookup(kern_pgdir, (void *)va, &pte_pr) == NULL) {
					cprintf("va 0x%08x has no physical mapping page\n\n", va);
					continue;
				}
				perm_set(pte_pr, perm);
			}
			i--;
		}
		else if (strcmp(argv[i], "-d") == 0) {
			// set or clean permission on PTE
			i++;
			uint32_t pdt_index = (uint32_t)strtol(argv[i], NULL, 16);
			uint32_t pte_index = 0;
			if ((kern_pgdir[pdt_index] & PTE_P) == 0) {
				cprintf("no page table in PD[%x]\n", pdt_index);
				return 0;
			}
			for (i = i + 1; i < argc; ++i) {
				if (argv[i][0] == '-') {
					break;
				}
				pte_index = (uint32_t)strtol(argv[i], NULL, 16);
				pte_t * pte_pr = (pte_t *)KADDR(PTE_ADDR(kern_pgdir[pdt_index])) + pte_index;
				perm_set(pte_pr, perm);
			}
			i--;
		}
		else {
			perm_help();
			return 0;
		}
	}
	return 0;
}

int mon_memory(int argc, char **argv, struct Trapframe *tf) {
	// check argc
	if (argc < 2) {
		cprintf("Too little arguments in memory command\n\n");
		memory_help();
		return 0;
	}

	if (strcmp(argv[1], "-h") == 0) {
		memory_help();
	}
	else if (strcmp(argv[1], "-v") == 0) {
		// show virtual memory
		if (argc != 4) {
			cprintf("Wrong arguments in memory command\n\n");
			memory_help();
			return 0;
		}
		uintptr_t va = (uintptr_t)strtol(argv[2], NULL, 16);
		uint32_t byte = (uint32_t)strtol(argv[3], NULL, 10);
		memory_virtual(va, byte);
	}
	else if (strcmp(argv[1], "-p") == 0) {
		// show physical memory
		if (argc != 4) {
			cprintf("Wrong arguments in memory command\n\n");
			memory_help();
			return 0;
		}
		physaddr_t pa = (physaddr_t)strtol(argv[2], NULL, 16);
		uint32_t byte = (uint32_t) strtol(argv[3], NULL, 10);
		memory_physical(pa, byte);
	}
	else {
		memory_help();
	}
	return 0;
}

void showmappings_help() {
	cprintf("Showmappings displays virtual memory and physical memory mapping relationship.\n");
	cprintf("-h:                 help\n");
	cprintf("-a [va1] [va2]:  display the physical page mappings and corresponding permission bits that apply to the pages at virtual address from va1 to va2\n");
	cprintf("\n");
}

void showmappings_vaddr(uintptr_t start, uintptr_t end) {
	cprintf("    VM            Entry        Ppage  Permission      PM\n");
	for (uintptr_t va = start; va < end; va += PGSIZE) {
			pte_t *pte_pr;
			struct PageInfo *pp = page_lookup(kern_pgdir, (void *)va, &pte_pr);
			if (pp == NULL) {
				cprintf("0x%08x\n", va);
				continue;
			}
			uint32_t pde_index = PDX(va);
			uint32_t pte_index = PTX(va);

			physaddr_t paddr = PTE_ADDR(*pte_pr) | PGOFF(va);
			cprintf("0x%08x  PDE[%03x] PTE[%03x]   %3x   ", va, pde_index, pte_index, pp - pages);
			showmappings_permission(pte_pr);
			cprintf("         0x%08x", paddr);
			cprintf("\n");
		}
}

void showmappings_permission(pte_t *pte_pr) {
	if (*pte_pr & PTE_U) {
		cprintf("u");
	}
	else {
		cprintf("-");
	}

	if (*pte_pr & PTE_W) {
		cprintf("rw");
	}
	else {
		cprintf("r-");
	}
}

void perm_help() {
	cprintf("Perm sets or change the permission of mappings.\n");
	cprintf("-h:                  help\n");
	cprintf("-s [perm]:           set mapping permission the \"perm\"\n");
	cprintf("-c:                  clean mapping permission to -rw\n");
	cprintf("-a [va1] [va2] ...:  set, clean or change va1, va2, ...'s mapping permission\n");
	cprintf("-d [pde] [pte1] ...: set, clean or change PD[pde] PT[pte]'s mapping permission\n\n");
	cprintf("You should use perm command in two ways:\n");
	cprintf("1. perm {-s [perm]/-c} -a [va1] [va2] ...\n");
	cprintf("2. perm {-s [perm]/-c} -d [pde] [pte1] [pte2] ...\n");
	cprintf("[perm] can be set by \"urw\", etc\n");
	cprintf("NOTE! You should not use -s/-c after -a/-d\n");
	cprintf("\n");
}

int perm_get(char *argv) {
	int perm = 0;
	for (int i = 0; argv[i] != '\0'; ++i) {
		if (argv[i] == 'u') {
			perm |= PTE_U;
		}
		if (argv[i] == 'w') {
			perm |= PTE_W;
		}
	}
	return perm;
}

void perm_set(pte_t *pte_pr, int perm) {
	*pte_pr &= ~0xFFF;
	*pte_pr |= (perm | PTE_P);  // Cautious!! 
								// Must set PTE_P, or this page isn't present and cannot access
}

void memory_help() {
	cprintf("Memory displays virtual memory or physical memory content.\n");
	cprintf("-h:       help\n");
	cprintf("-v [va] [b]:  display b byte virtual memory va's content\n");
	cprintf("-p [pa] [b]:  display b byte physical memory pa's content\n");
	cprintf("\n");
}

void memory_virtual(uintptr_t va, uint32_t byte) {
	pte_t *pte_pr;
	struct PageInfo *pp = page_lookup(kern_pgdir, (void *)va, &pte_pr);
	if (pp == NULL || pte_pr == NULL || (*pte_pr & PTE_P) == 0) {
		cprintf("0x%08x:  Cannot access memory at address 0x%08x\n", va, va);
		return;
	}
	if (byte == 0) {
		return;
	}
	for (int i = 0; i < byte; ++i) {
		if (i % 4 == 0) {
			cprintf("0x%08x: ", va + i);
		}
		cprintf(" 0x%08x", *((uint32_t *)va + i));
		if (i % 4 == 3) {
			cprintf("\n");
		}
	}
}

void memory_physical(physaddr_t pa, uint32_t byte) {
	uintptr_t va = (uintptr_t)KADDR(pa);
	memory_virtual(va, byte);
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("%C__       __   %C_______    %C_______    %C________\n", LIGHT_MAGENTA, LIGHT_RED, YELLOW, GREEN);
	cprintf("%C\\ \\     / /  %C|  _____|  %C|  ___  |  %C|  ____  |\n", LIGHT_MAGENTA, LIGHT_RED, YELLOW, GREEN);
	cprintf(" %C\\ \\   / /   %C| |_____   %C| |___| |  %C| |    | |\n", LIGHT_MAGENTA, LIGHT_RED, YELLOW, GREEN);
	cprintf("  %C\\ \\_/ /    %C|  _____|  %C|  _  __|  %C| |    | |\n", LIGHT_MAGENTA, LIGHT_RED, YELLOW, GREEN);
	cprintf("   %C\\   /     %C| |_____   %C| | \\ \\    %C| |____| |\n", LIGHT_MAGENTA, LIGHT_RED, YELLOW, GREEN);
	cprintf("    %C\\_/      %C|_______|  %C|_|  \\_\\   %C|________|\n", LIGHT_MAGENTA, LIGHT_RED, YELLOW, GREEN);
	cprintf("\n%C", LIGHT_GRAY);
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
