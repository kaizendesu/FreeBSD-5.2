/*-
 * Copyright (c) 2000,2001 Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include "opt_compat.h"
#include "opt_ddb.h"
#include "opt_kstack_pages.h"
#include "opt_msgbuf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/eventhandler.h>
#include <sys/sysproto.h>
#include <sys/signalvar.h>
#include <sys/imgact.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/pcpu.h>
#include <sys/malloc.h>
#include <sys/reboot.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/vmmeter.h>
#include <sys/msgbuf.h>
#include <sys/exec.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/linker.h>
#include <sys/random.h>
#include <sys/cons.h>
#include <sys/uuid.h>
#include <sys/syscall.h>
#include <net/netisr.h>
#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>
#include <vm/vm_extern.h>
#include <vm/vm_object.h>
#include <vm/vm_pager.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/md_var.h>
#include <machine/reg.h>
#include <machine/fpu.h>
#include <machine/mca.h>
#include <machine/pal.h>
#include <machine/sal.h>
#ifdef SMP
#include <machine/smp.h>
#endif
#include <machine/bootinfo.h>
#include <machine/mutex.h>
#include <machine/vmparam.h>
#include <machine/elf.h>
#include <ddb/ddb.h>
#include <sys/vnode.h>
#include <sys/ucontext.h>
#include <machine/sigframe.h>
#include <machine/efi.h>
#include <machine/unwind.h>
#include <i386/include/specialreg.h>

u_int64_t processor_frequency;
u_int64_t bus_frequency;
u_int64_t itc_frequency;
int cold = 1;

u_int64_t pa_bootinfo;
struct bootinfo bootinfo;

struct pcpu early_pcpu;
extern char kstack[]; 
struct user *proc0uarea;
vm_offset_t proc0kstack;

extern u_int64_t kernel_text[], _end[];

extern u_int64_t ia64_gateway_page[];
extern u_int64_t break_sigtramp[];
extern u_int64_t epc_sigtramp[];

FPSWA_INTERFACE *fpswa_interface;

u_int64_t ia64_pal_base;
u_int64_t ia64_port_base;

char machine[] = MACHINE;
SYSCTL_STRING(_hw, HW_MACHINE, machine, CTLFLAG_RD, machine, 0, "");

static char cpu_model[64];
SYSCTL_STRING(_hw, HW_MODEL, model, CTLFLAG_RD, cpu_model, 0,
    "The CPU model name");

static char cpu_family[64];
SYSCTL_STRING(_hw, OID_AUTO, family, CTLFLAG_RD, cpu_family, 0,
    "The CPU family name");

#ifdef DDB
/* start and end of kernel symbol table */
void	*ksym_start, *ksym_end;
#endif

static void cpu_startup(void *);
SYSINIT(cpu, SI_SUB_CPU, SI_ORDER_FIRST, cpu_startup, NULL)

struct msgbuf *msgbufp=0;

long Maxmem = 0;

vm_offset_t phys_avail[100];

/* must be 2 less so 0 0 can signal end of chunks */
#define PHYS_AVAIL_ARRAY_END ((sizeof(phys_avail) / sizeof(vm_offset_t)) - 2)

void mi_startup(void);		/* XXX should be in a MI header */

struct kva_md_info kmi;

static void
identifycpu(void)
{
	char vendor[17];
	char *family_name, *model_name;
	u_int64_t t;
	int number, revision, model, family, archrev;
	u_int64_t features;

	/*
	 * Assumes little-endian.
	 */
	*(u_int64_t *) &vendor[0] = ia64_get_cpuid(0);
	*(u_int64_t *) &vendor[8] = ia64_get_cpuid(1);
	vendor[16] = '\0';

	t = ia64_get_cpuid(3);
	number = (t >> 0) & 0xff;
	revision = (t >> 8) & 0xff;
	model = (t >> 16) & 0xff;
	family = (t >> 24) & 0xff;
	archrev = (t >> 32) & 0xff;

	family_name = model_name = "unknown";
	switch (family) {
	case 0x07:
		family_name = "Itanium";
		model_name = "Merced";
		break;
	case 0x1f:
		family_name = "Itanium 2";
		switch (model) {
		case 0x00:
			model_name = "McKinley";
			break;
		case 0x01:
			model_name = "Madison";
			break;
		}
		break;
	}
	snprintf(cpu_family, sizeof(cpu_family), "%s", family_name);
	snprintf(cpu_model, sizeof(cpu_model), "%s", model_name);

	features = ia64_get_cpuid(4);

	printf("CPU: %s (", model_name);
	if (processor_frequency) {
		printf("%ld.%02ld-Mhz ",
		    (processor_frequency + 4999) / 1000000,
		    ((processor_frequency + 4999) / 10000) % 100);
	}
	printf("%s)\n", family_name);
	printf("  Origin = \"%s\"  Revision = %d\n", vendor, revision);
	printf("  Features = 0x%b\n", (u_int32_t) features,
	    "\020"
	    "\001LB"	/* long branch (brl) instruction. */
	    "\002SD"	/* Spontaneous deferral. */
	    "\003AO"	/* 16-byte atomic operations (ld, st, cmpxchg). */ );
}

static void
cpu_startup(dummy)
	void *dummy;
{

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	identifycpu();

	/* startrtclock(); */
#ifdef PERFMON
	perfmon_init();
#endif
	printf("real memory  = %ld (%ld MB)\n", ia64_ptob(Maxmem),
	    ia64_ptob(Maxmem) / 1048576);

	/*
	 * Display any holes after the first chunk of extended memory.
	 */
	if (bootverbose) {
		int indx;

		printf("Physical memory chunk(s):\n");
		for (indx = 0; phys_avail[indx + 1] != 0; indx += 2) {
			int size1 = phys_avail[indx + 1] - phys_avail[indx];

			printf("0x%08lx - 0x%08lx, %d bytes (%d pages)\n", phys_avail[indx],
			    phys_avail[indx + 1] - 1, size1, size1 / PAGE_SIZE);
		}
	}

	vm_ksubmap_init(&kmi);

	printf("avail memory = %ld (%ld MB)\n", ptoa(cnt.v_free_count),
	    ptoa(cnt.v_free_count) / 1048576);
 
	if (fpswa_interface == NULL)
		printf("Warning: no FPSWA package supplied\n");
	else
		printf("FPSWA Revision = 0x%lx, Entry = %p\n",
		    (long)fpswa_interface->Revision,
		    (void *)fpswa_interface->Fpswa);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
	vm_pager_bufferinit();

	/*
	 * Traverse the MADT to discover IOSAPIC and Local SAPIC
	 * information.
	 */
	ia64_probe_sapics();
	ia64_mca_init();
}

void
cpu_boot(int howto)
{

	ia64_efi_runtime->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, 0);
}

void
cpu_halt()
{

	ia64_efi_runtime->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, 0);
}

static void
cpu_idle_default(void)
{
	struct ia64_pal_result res;

	res = ia64_call_pal_static(PAL_HALT_LIGHT, 0, 0, 0);
}

void
cpu_idle()
{
	(*cpu_idle_hook)();
}

/* Other subsystems (e.g., ACPI) can hook this later. */
void (*cpu_idle_hook)(void) = cpu_idle_default;

void
cpu_reset()
{

	cpu_boot(0);
}

void
cpu_switch(struct thread *old, struct thread *new)
{
	struct pcb *oldpcb, *newpcb;

	oldpcb = old->td_pcb;
#if IA32
	ia32_savectx(oldpcb);
#endif
	if (PCPU_GET(fpcurthread) == old)
		old->td_frame->tf_special.psr |= IA64_PSR_DFH;
	if (!savectx(oldpcb)) {
		newpcb = new->td_pcb;
		oldpcb->pcb_current_pmap =
		    pmap_switch(newpcb->pcb_current_pmap);
		PCPU_SET(curthread, new);
#if IA32
		ia32_restorectx(newpcb);
#endif
		if (PCPU_GET(fpcurthread) == new)
			new->td_frame->tf_special.psr &= ~IA64_PSR_DFH;
		restorectx(newpcb);
		/* We should not get here. */
		panic("cpu_switch: restorectx() returned");
		/* NOTREACHED */
	}
}

void
cpu_throw(struct thread *old __unused, struct thread *new)
{
	struct pcb *newpcb;

	newpcb = new->td_pcb;
	(void)pmap_switch(newpcb->pcb_current_pmap);
	PCPU_SET(curthread, new);
#if IA32
	ia32_restorectx(newpcb);
#endif
	restorectx(newpcb);
	/* We should not get here. */
	panic("cpu_throw: restorectx() returned");
	/* NOTREACHED */
}

void
cpu_pcpu_init(struct pcpu *pcpu, int cpuid, size_t size)
{
	size_t pcpusz;

	/*
	 * Make sure the PCB is 16-byte aligned by making the PCPU
	 * a multiple of 16 bytes. We assume the PCPU is 16-byte
	 * aligned itself.
	 */
	pcpusz = (sizeof(struct pcpu) + 15) & ~15;
	KASSERT(size >= pcpusz + sizeof(struct pcb),
	    ("%s: too small an allocation for pcpu", __func__));
	pcpu->pc_pcb = (struct pcb *)((char*)pcpu + pcpusz);
	pcpu->pc_acpi_id = cpuid;
}

void
map_pal_code(void)
{
	struct ia64_pte pte;
	u_int64_t psr;

	if (ia64_pal_base == 0)
		return;

	bzero(&pte, sizeof(pte));
	pte.pte_p = 1;
	pte.pte_ma = PTE_MA_WB;
	pte.pte_a = 1;
	pte.pte_d = 1;
	pte.pte_pl = PTE_PL_KERN;
	pte.pte_ar = PTE_AR_RWX;
	pte.pte_ppn = ia64_pal_base >> 12;

	__asm __volatile("ptr.d %0,%1; ptr.i %0,%1" ::
	    "r"(IA64_PHYS_TO_RR7(ia64_pal_base)), "r"(IA64_ID_PAGE_SHIFT<<2));

	__asm __volatile("mov	%0=psr" : "=r"(psr));
	__asm __volatile("rsm	psr.ic|psr.i");
	__asm __volatile("srlz.i");
	__asm __volatile("mov	cr.ifa=%0" ::
	    "r"(IA64_PHYS_TO_RR7(ia64_pal_base)));
	__asm __volatile("mov	cr.itir=%0" :: "r"(IA64_ID_PAGE_SHIFT << 2));
	__asm __volatile("itr.d	dtr[%0]=%1" :: "r"(1), "r"(*(u_int64_t*)&pte));
	__asm __volatile("srlz.d");		/* XXX not needed. */
	__asm __volatile("itr.i	itr[%0]=%1" :: "r"(1), "r"(*(u_int64_t*)&pte));
	__asm __volatile("mov	psr.l=%0" :: "r" (psr));
	__asm __volatile("srlz.i");
}

void
map_port_space(void)
{
	struct ia64_pte pte;
	u_int64_t psr;

	/* XXX we should fail hard if there's no I/O port space. */
	if (ia64_port_base == 0)
		return;

	bzero(&pte, sizeof(pte));
	pte.pte_p = 1;
	pte.pte_ma = PTE_MA_UC;
	pte.pte_a = 1;
	pte.pte_d = 1;
	pte.pte_pl = PTE_PL_KERN;
	pte.pte_ar = PTE_AR_RW;
	pte.pte_ppn = ia64_port_base >> 12;

	__asm __volatile("ptr.d %0,%1" :: "r"(ia64_port_base), "r"(24 << 2));

	__asm __volatile("mov	%0=psr" : "=r" (psr));
	__asm __volatile("rsm	psr.ic|psr.i");
	__asm __volatile("srlz.d");
	__asm __volatile("mov	cr.ifa=%0" :: "r"(ia64_port_base));
	__asm __volatile("mov	cr.itir=%0" :: "r"(IA64_ID_PAGE_SHIFT << 2));
	__asm __volatile("itr.d dtr[%0]=%1" :: "r"(2), "r"(*(u_int64_t*)&pte));
	__asm __volatile("mov	psr.l=%0" :: "r" (psr));
	__asm __volatile("srlz.d");
}

void
map_gateway_page(void)
{
	struct ia64_pte pte;
	u_int64_t psr;

	bzero(&pte, sizeof(pte));
	pte.pte_p = 1;
	pte.pte_ma = PTE_MA_WB;
	pte.pte_a = 1;
	pte.pte_d = 1;
	pte.pte_pl = PTE_PL_KERN;
	pte.pte_ar = PTE_AR_X_RX;
	pte.pte_ppn = IA64_RR_MASK((u_int64_t)ia64_gateway_page) >> 12;

	__asm __volatile("ptr.d %0,%1; ptr.i %0,%1" ::
	    "r"(VM_MAX_ADDRESS), "r"(PAGE_SHIFT << 2));

	__asm __volatile("mov	%0=psr" : "=r"(psr));
	__asm __volatile("rsm	psr.ic|psr.i");
	__asm __volatile("srlz.i");
	__asm __volatile("mov	cr.ifa=%0" :: "r"(VM_MAX_ADDRESS));
	__asm __volatile("mov	cr.itir=%0" :: "r"(PAGE_SHIFT << 2));
	__asm __volatile("itr.d	dtr[%0]=%1" :: "r"(3), "r"(*(u_int64_t*)&pte));
	__asm __volatile("srlz.d");		/* XXX not needed. */
	__asm __volatile("itr.i	itr[%0]=%1" :: "r"(3), "r"(*(u_int64_t*)&pte));
	__asm __volatile("mov	psr.l=%0" :: "r" (psr));
	__asm __volatile("srlz.i");

	/* Expose the mapping to userland in ar.k5 */
	ia64_set_k5(VM_MAX_ADDRESS);
}

static void
calculate_frequencies(void)
{
	struct ia64_sal_result sal;
	struct ia64_pal_result pal;

	sal = ia64_sal_entry(SAL_FREQ_BASE, 0, 0, 0, 0, 0, 0, 0);
	pal = ia64_call_pal_static(PAL_FREQ_RATIOS, 0, 0, 0);

	if (sal.sal_status == 0 && pal.pal_status == 0) {
		if (bootverbose) {
			printf("Platform clock frequency %ld Hz\n",
			       sal.sal_result[0]);
			printf("Processor ratio %ld/%ld, Bus ratio %ld/%ld, "
			       "ITC ratio %ld/%ld\n",
			       pal.pal_result[0] >> 32,
			       pal.pal_result[0] & ((1L << 32) - 1),
			       pal.pal_result[1] >> 32,
			       pal.pal_result[1] & ((1L << 32) - 1),
			       pal.pal_result[2] >> 32,
			       pal.pal_result[2] & ((1L << 32) - 1));
		}
		processor_frequency =
			sal.sal_result[0] * (pal.pal_result[0] >> 32)
			/ (pal.pal_result[0] & ((1L << 32) - 1));
		bus_frequency =
			sal.sal_result[0] * (pal.pal_result[1] >> 32)
			/ (pal.pal_result[1] & ((1L << 32) - 1));
		itc_frequency =
			sal.sal_result[0] * (pal.pal_result[2] >> 32)
			/ (pal.pal_result[2] & ((1L << 32) - 1));
	}
}

void
ia64_init(void)
{
	int phys_avail_cnt;
	vm_offset_t kernstart, kernend;
	vm_offset_t kernstartpfn, kernendpfn, pfn0, pfn1;
	char *p;
	EFI_MEMORY_DESCRIPTOR *md, *mdp;
	int mdcount, i, metadata_missing;

	/* NO OUTPUT ALLOWED UNTIL FURTHER NOTICE */

	/*
	 * TODO: Disable interrupts, floating point etc.
	 * Maybe flush cache and tlb
	 */
	ia64_set_fpsr(IA64_FPSR_DEFAULT);

	/*
	 * TODO: Get critical system information (if possible, from the
	 * information provided by the boot program).
	 */

	/*
	 * pa_bootinfo is the physical address of the bootinfo block as
	 * passed to us by the loader and set in locore.s.
	 */
	bootinfo = *(struct bootinfo *)(IA64_PHYS_TO_RR7(pa_bootinfo));

	if (bootinfo.bi_magic != BOOTINFO_MAGIC || bootinfo.bi_version != 1) {
		bzero(&bootinfo, sizeof(bootinfo));
		bootinfo.bi_kernend = (vm_offset_t) round_page(_end);
	}

	/*
	 * Look for the I/O ports first - we need them for console
	 * probing.
	 */
	mdcount = bootinfo.bi_memmap_size / bootinfo.bi_memdesc_size;
	md = (EFI_MEMORY_DESCRIPTOR *) IA64_PHYS_TO_RR7(bootinfo.bi_memmap);

	for (i = 0, mdp = md; i < mdcount; i++,
	    mdp = NextMemoryDescriptor(mdp, bootinfo.bi_memdesc_size)) {
		if (mdp->Type == EfiMemoryMappedIOPortSpace)
			ia64_port_base = IA64_PHYS_TO_RR6(mdp->PhysicalStart);
		else if (mdp->Type == EfiPalCode)
			ia64_pal_base = mdp->PhysicalStart;
	}

	map_port_space();

	metadata_missing = 0;
	if (bootinfo.bi_modulep)
		preload_metadata = (caddr_t)bootinfo.bi_modulep;
	else
		metadata_missing = 1;
	if (envmode == 1)
		kern_envp = static_env;
	else
		kern_envp = (caddr_t)bootinfo.bi_envp;

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = bootinfo.bi_boothowto;

	/*
	 * Catch case of boot_verbose set in environment.
	 */
	if ((p = getenv("boot_verbose")) != NULL) {
		if (strcmp(p, "yes") == 0 || strcmp(p, "YES") == 0) {
			boothowto |= RB_VERBOSE;
		}
		freeenv(p);
	}

	if (boothowto & RB_VERBOSE)
		bootverbose = 1;

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

	/* OUTPUT NOW ALLOWED */

	if (ia64_pal_base != 0) {
		ia64_pal_base &= ~IA64_ID_PAGE_MASK;
		/*
		 * We use a TR to map the first 256M of memory - this might
		 * cover the palcode too.
		 */
		if (ia64_pal_base == 0)
			printf("PAL code mapped by the kernel's TR\n");
	} else
		printf("PAL code not found\n");

	/*
	 * Wire things up so we can call the firmware.
	 */
	map_pal_code();
	ia64_efi_init();
	calculate_frequencies();

	/*
	 * Find the beginning and end of the kernel.
	 */
	kernstart = trunc_page(kernel_text);
#ifdef DDB
	ksym_start = (void *)bootinfo.bi_symtab;
	ksym_end = (void *)bootinfo.bi_esymtab;
	kernend = (vm_offset_t)round_page(ksym_end);
#else
	kernend = (vm_offset_t)round_page(_end);
#endif

	/* But if the bootstrap tells us otherwise, believe it! */
	if (bootinfo.bi_kernend)
		kernend = round_page(bootinfo.bi_kernend);
	if (metadata_missing)
		printf("WARNING: loader(8) metadata is missing!\n");

	/* Get FPSWA interface */
	fpswa_interface = (FPSWA_INTERFACE*)IA64_PHYS_TO_RR7(bootinfo.bi_fpswa);

	/* Init basic tunables, including hz */
	init_param1();

	p = getenv("kernelname");
	if (p) {
		strncpy(kernelname, p, sizeof(kernelname) - 1);
		freeenv(p);
	}

	kernstartpfn = atop(IA64_RR_MASK(kernstart));
	kernendpfn = atop(IA64_RR_MASK(kernend));

	/*
	 * Size the memory regions and load phys_avail[] with the results.
	 */

	/*
	 * Find out how much memory is available, by looking at
	 * the memory descriptors.
	 */

#ifdef DEBUG_MD
	printf("Memory descriptor count: %d\n", mdcount);
#endif

	phys_avail_cnt = 0;
	for (i = 0, mdp = md; i < mdcount; i++,
		 mdp = NextMemoryDescriptor(mdp, bootinfo.bi_memdesc_size)) {
#ifdef DEBUG_MD
		printf("MD %d: type %d pa 0x%lx cnt 0x%lx\n", i,
		       mdp->Type,
		       mdp->PhysicalStart,
		       mdp->NumberOfPages);
#endif

		pfn0 = ia64_btop(round_page(mdp->PhysicalStart));
		pfn1 = ia64_btop(trunc_page(mdp->PhysicalStart
					    + mdp->NumberOfPages * 4096));
		if (pfn1 <= pfn0)
			continue;

		if (mdp->Type != EfiConventionalMemory)
			continue;

		/*
		 * Wimp out for now since we do not DTRT here with
		 * pci bus mastering (no bounce buffering, for example).
		 */
		if (pfn0 >= ia64_btop(0x100000000UL)) {
			printf("Skipping memory chunk start 0x%lx\n",
			    mdp->PhysicalStart);
			continue;
		}
		if (pfn1 >= ia64_btop(0x100000000UL)) {
			printf("Skipping memory chunk end 0x%lx\n",
			    mdp->PhysicalStart + mdp->NumberOfPages * 4096);
			continue;
		}

		/*
		 * We have a memory descriptor that describes conventional
		 * memory that is for general use. We must determine if the
		 * loader has put the kernel in this region.
		 */
		physmem += (pfn1 - pfn0);
		if (pfn0 <= kernendpfn && kernstartpfn <= pfn1) {
			/*
			 * Must compute the location of the kernel
			 * within the segment.
			 */
#ifdef DEBUG_MD
			printf("Descriptor %d contains kernel\n", i);
#endif
			if (pfn0 < kernstartpfn) {
				/*
				 * There is a chunk before the kernel.
				 */
#ifdef DEBUG_MD
				printf("Loading chunk before kernel: "
				       "0x%lx / 0x%lx\n", pfn0, kernstartpfn);
#endif
				phys_avail[phys_avail_cnt] = ia64_ptob(pfn0);
				phys_avail[phys_avail_cnt+1] = ia64_ptob(kernstartpfn);
				phys_avail_cnt += 2;
			}
			if (kernendpfn < pfn1) {
				/*
				 * There is a chunk after the kernel.
				 */
#ifdef DEBUG_MD
				printf("Loading chunk after kernel: "
				       "0x%lx / 0x%lx\n", kernendpfn, pfn1);
#endif
				phys_avail[phys_avail_cnt] = ia64_ptob(kernendpfn);
				phys_avail[phys_avail_cnt+1] = ia64_ptob(pfn1);
				phys_avail_cnt += 2;
			}
		} else {
			/*
			 * Just load this cluster as one chunk.
			 */
#ifdef DEBUG_MD
			printf("Loading descriptor %d: 0x%lx / 0x%lx\n", i,
			       pfn0, pfn1);
#endif
			phys_avail[phys_avail_cnt] = ia64_ptob(pfn0);
			phys_avail[phys_avail_cnt+1] = ia64_ptob(pfn1);
			phys_avail_cnt += 2;
			
		}
	}
	phys_avail[phys_avail_cnt] = 0;

	Maxmem = physmem;
	init_param2(physmem);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	msgbufp = (struct msgbuf *)pmap_steal_memory(MSGBUF_SIZE);
	msgbufinit(msgbufp, MSGBUF_SIZE);

	proc_linkup(&proc0, &ksegrp0, &kse0, &thread0);
	/*
	 * Init mapping for u page(s) for proc 0
	 */
	proc0uarea = (struct user *)pmap_steal_memory(UAREA_PAGES * PAGE_SIZE);
	proc0kstack = (vm_offset_t)kstack;
	proc0.p_uarea = proc0uarea;
	thread0.td_kstack = proc0kstack;
	thread0.td_pcb = (struct pcb *)
	    (thread0.td_kstack + KSTACK_PAGES * PAGE_SIZE) - 1;
	/*
	 * Setup the global data for the bootstrap cpu.
	 */
	pcpup = (struct pcpu *)pmap_steal_memory(PAGE_SIZE);
	ia64_set_k4((u_int64_t)pcpup);
	pcpu_init(pcpup, 0, PAGE_SIZE);
	PCPU_SET(curthread, &thread0);

	/*
	 * Initialize the rest of proc 0's PCB.
	 *
	 * Set the kernel sp, reserving space for an (empty) trapframe,
	 * and make proc0's trapframe pointer point to it for sanity.
	 * Initialise proc0's backing store to start after u area.
	 */
	thread0.td_frame = (struct trapframe *)thread0.td_pcb - 1;
	thread0.td_frame->tf_length = sizeof(struct trapframe);
	thread0.td_frame->tf_flags = FRAME_SYSCALL;
	thread0.td_pcb->pcb_special.sp =
	    (u_int64_t)thread0.td_frame - 16;
	thread0.td_pcb->pcb_special.bspstore = (u_int64_t)proc0kstack;

	mutex_init();

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Initialize debuggers, and break into them if appropriate.
	 */
#ifdef DDB
	kdb_init();
	if (boothowto & RB_KDB) {
		printf("Boot flags requested debugger\n");
		breakpoint();
	}
#endif
	ia64_set_tpr(0);

	/*
	 * Save our current context so that we have a known (maybe even
	 * sane) context as the initial context for new threads that are
	 * forked from us. If any of those threads (including thread0)
	 * does something wrong, we may be lucky and return here where
	 * we're ready for them with a nice panic.
	 */
	if (!savectx(thread0.td_pcb))
		mi_startup();

	/* We should not get here. */
	panic("ia64_init: Whooaa there!");
	/* NOTREACHED */
}

void
bzero(void *buf, size_t len)
{
	caddr_t p = buf;

	while (((vm_offset_t) p & (sizeof(u_long) - 1)) && len) {
		*p++ = 0;
		len--;
	}
	while (len >= sizeof(u_long) * 8) {
		*(u_long*) p = 0;
		*((u_long*) p + 1) = 0;
		*((u_long*) p + 2) = 0;
		*((u_long*) p + 3) = 0;
		len -= sizeof(u_long) * 8;
		*((u_long*) p + 4) = 0;
		*((u_long*) p + 5) = 0;
		*((u_long*) p + 6) = 0;
		*((u_long*) p + 7) = 0;
		p += sizeof(u_long) * 8;
	}
	while (len >= sizeof(u_long)) {
		*(u_long*) p = 0;
		len -= sizeof(u_long);
		p += sizeof(u_long);
	}
	while (len) {
		*p++ = 0;
		len--;
	}
}

void
DELAY(int n)
{
	u_int64_t start, end, now;

	start = ia64_get_itc();
	end = start + (itc_frequency * n) / 1000000;
	/* printf("DELAY from 0x%lx to 0x%lx\n", start, end); */
	do {
		now = ia64_get_itc();
	} while (now < end || (now > start && end < start));
}

/*
 * Send an interrupt (signal) to a process.
 */
void
sendsig(sig_t catcher, int sig, sigset_t *mask, u_long code)
{
	struct proc *p;
	struct thread *td;
	struct trapframe *tf;
	struct sigacts *psp;
	struct sigframe sf, *sfp;
	u_int64_t sbs, sp;
	int oonstack;

	td = curthread;
	p = td->td_proc;
	PROC_LOCK_ASSERT(p, MA_OWNED);
	psp = p->p_sigacts;
	mtx_assert(&psp->ps_mtx, MA_OWNED);
	tf = td->td_frame;
	sp = tf->tf_special.sp;
	oonstack = sigonstack(sp);
	sbs = 0;

	/* save user context */
	bzero(&sf, sizeof(struct sigframe));
	sf.sf_uc.uc_sigmask = *mask;
	sf.sf_uc.uc_stack = p->p_sigstk;
	sf.sf_uc.uc_stack.ss_flags = (p->p_flag & P_ALTSTACK)
	    ? ((oonstack) ? SS_ONSTACK : 0) : SS_DISABLE;

	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if ((p->p_flag & P_ALTSTACK) != 0 && !oonstack &&
	    SIGISMEMBER(psp->ps_sigonstack, sig)) {
		sbs = (u_int64_t)p->p_sigstk.ss_sp;
		sbs = (sbs + 15) & ~15;
		sfp = (struct sigframe *)(sbs + p->p_sigstk.ss_size);
#if defined(COMPAT_43) || defined(COMPAT_SUNOS)
		p->p_sigstk.ss_flags |= SS_ONSTACK;
#endif
	} else
		sfp = (struct sigframe *)sp;
	sfp = (struct sigframe *)((u_int64_t)(sfp - 1) & ~15);

	/* Fill in the siginfo structure for POSIX handlers. */
	if (SIGISMEMBER(psp->ps_siginfo, sig)) {
		sf.sf_si.si_signo = sig;
		sf.sf_si.si_code = code;
		sf.sf_si.si_addr = (void*)tf->tf_special.ifa;
		code = (u_int64_t)&sfp->sf_si;
	}

	mtx_unlock(&psp->ps_mtx);
	PROC_UNLOCK(p);

	get_mcontext(td, &sf.sf_uc.uc_mcontext, GET_MC_IA64_SCRATCH);

	/* Copy the frame out to userland. */
	if (copyout(&sf, sfp, sizeof(sf)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		PROC_LOCK(p);
		sigexit(td, SIGILL);
		return;
	}

	if ((tf->tf_flags & FRAME_SYSCALL) == 0) {
		tf->tf_special.psr &= ~IA64_PSR_RI;
		tf->tf_special.iip = ia64_get_k5() +
		    ((uint64_t)break_sigtramp - (uint64_t)ia64_gateway_page);
	} else
		tf->tf_special.iip = ia64_get_k5() +
		    ((uint64_t)epc_sigtramp - (uint64_t)ia64_gateway_page);

	/*
	 * Setup the trapframe to return to the signal trampoline. We pass
	 * information to the trampoline in the following registers:
	 *
	 *	gp	new backing store or NULL
	 *	r8	signal number
	 *	r9	signal code or siginfo pointer
	 *	r10	signal handler (function descriptor)
	 */
	tf->tf_special.sp = (u_int64_t)sfp - 16;
	tf->tf_special.gp = sbs;
	tf->tf_special.bspstore = sf.sf_uc.uc_mcontext.mc_special.bspstore;
	tf->tf_special.ndirty = 0;
	tf->tf_special.rnat = sf.sf_uc.uc_mcontext.mc_special.rnat;
	tf->tf_scratch.gr8 = sig;
	tf->tf_scratch.gr9 = code;
	tf->tf_scratch.gr10 = (u_int64_t)catcher;

	PROC_LOCK(p);
	mtx_lock(&psp->ps_mtx);
}

/*
 * Build siginfo_t for SA thread
 */
void
cpu_thread_siginfo(int sig, u_long code, siginfo_t *si)
{
	struct proc *p;
	struct thread *td;

	td = curthread;
	p = td->td_proc;
	PROC_LOCK_ASSERT(p, MA_OWNED);

	bzero(si, sizeof(*si));
	si->si_signo = sig;
	si->si_code = code;
	/* XXXKSE fill other fields */
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * state to gain improper privileges.
 *
 * MPSAFE
 */
int
sigreturn(struct thread *td,
	struct sigreturn_args /* {
		ucontext_t *sigcntxp;
	} */ *uap)
{
	ucontext_t uc;
	struct trapframe *tf;
	struct proc *p;
	struct pcb *pcb;

	tf = td->td_frame;
	p = td->td_proc;
	pcb = td->td_pcb;

	/*
	 * Fetch the entire context structure at once for speed.
	 * We don't use a normal argument to simplify RSE handling.
	 */
	if (copyin(uap->sigcntxp, (caddr_t)&uc, sizeof(uc)))
		return (EFAULT);

	set_mcontext(td, &uc.uc_mcontext);

	PROC_LOCK(p);
#if defined(COMPAT_43) || defined(COMPAT_SUNOS)
	if (sigonstack(tf->tf_special.sp))
		p->p_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigstk.ss_flags &= ~SS_ONSTACK;
#endif
	td->td_sigmask = uc.uc_sigmask;
	SIG_CANTMASK(td->td_sigmask);
	signotify(td);
	PROC_UNLOCK(p);

	return (EJUSTRETURN);
}

#ifdef COMPAT_FREEBSD4
int
freebsd4_sigreturn(struct thread *td, struct freebsd4_sigreturn_args *uap)
{

	return sigreturn(td, (struct sigreturn_args *)uap);
}
#endif

int
get_mcontext(struct thread *td, mcontext_t *mc, int flags)
{
	struct trapframe *tf;
	uint64_t bspst, kstk, rnat;

	tf = td->td_frame;
	bzero(mc, sizeof(*mc));
	if (tf->tf_special.ndirty != 0) {
		kstk = td->td_kstack + (tf->tf_special.bspstore & 0x1ffUL);
		__asm __volatile("mov	ar.rsc=0;;");
		__asm __volatile("mov	%0=ar.bspstore" : "=r"(bspst));
		/* Make sure we have all the user registers written out. */
		if (bspst - kstk < tf->tf_special.ndirty) {
			__asm __volatile("flushrs;;");
			__asm __volatile("mov	%0=ar.bspstore" : "=r"(bspst));
		}
		__asm __volatile("mov	%0=ar.rnat;;" : "=r"(rnat));
		__asm __volatile("mov	ar.rsc=3");
		copyout((void*)kstk, (void*)tf->tf_special.bspstore,
		    tf->tf_special.ndirty);
		kstk += tf->tf_special.ndirty;
		mc->mc_special = tf->tf_special;
		mc->mc_special.rnat =
		    (bspst > kstk && (bspst & 0x1ffUL) < (kstk & 0x1ffUL))
		    ? *(uint64_t*)(kstk | 0x1f8UL) : rnat;
		mc->mc_special.bspstore += mc->mc_special.ndirty;
		mc->mc_special.ndirty = 0;
	} else
		mc->mc_special = tf->tf_special;
	if (tf->tf_flags & FRAME_SYSCALL) {
		if (flags & GET_MC_IA64_SCRATCH) {
			mc->mc_flags |= _MC_FLAGS_SCRATCH_VALID;
			mc->mc_scratch = tf->tf_scratch;
		} else {
			/*
			 * Put the syscall return values in the context.  We
			 * need this for swapcontext() to work.  Note that we
			 * don't use gr11 in the kernel, but the runtime
			 * specification defines it as a return register,
			 * just like gr8-gr10.
			 */
			mc->mc_flags |= _MC_FLAGS_RETURN_VALID;
			if ((flags & GET_MC_CLEAR_RET) == 0) {
				mc->mc_scratch.gr8 = tf->tf_scratch.gr8;
				mc->mc_scratch.gr9 = tf->tf_scratch.gr9;
				mc->mc_scratch.gr10 = tf->tf_scratch.gr10;
				mc->mc_scratch.gr11 = tf->tf_scratch.gr11;
			}
		}
	} else {
		mc->mc_flags |= _MC_FLAGS_ASYNC_CONTEXT;
		mc->mc_scratch = tf->tf_scratch;
		mc->mc_scratch_fp = tf->tf_scratch_fp;
		/*
		 * XXX If the thread never used the high FP registers, we
		 * probably shouldn't waste time saving them.
		 */
		ia64_highfp_save(td);
		mc->mc_flags |= _MC_FLAGS_HIGHFP_VALID;
		mc->mc_high_fp = td->td_pcb->pcb_high_fp;
	}
	save_callee_saved(&mc->mc_preserved);
	save_callee_saved_fp(&mc->mc_preserved_fp);
	return (0);
}

int
set_mcontext(struct thread *td, const mcontext_t *mc)
{
	struct _special s;
	struct trapframe *tf;
	uint64_t psrmask;

	tf = td->td_frame;

	KASSERT((tf->tf_special.ndirty & ~PAGE_MASK) == 0,
	    ("Whoa there! We have more than 8KB of dirty registers!"));

	s = mc->mc_special;
	/*
	 * Only copy the user mask and the restart instruction bit from
	 * the new context.
	 */
	psrmask = IA64_PSR_BE | IA64_PSR_UP | IA64_PSR_AC | IA64_PSR_MFL |
	    IA64_PSR_MFH | IA64_PSR_RI;
	s.psr = (tf->tf_special.psr & ~psrmask) | (s.psr & psrmask);
	/* We don't have any dirty registers of the new context. */
	s.ndirty = 0;
	if (mc->mc_flags & _MC_FLAGS_ASYNC_CONTEXT) {
		/*
		 * We can get an async context passed to us while we
		 * entered the kernel through a syscall: sigreturn(2).
		 * Hence, we cannot assert that the trapframe is not
		 * a syscall frame, but we can assert that if it is
		 * the syscall is sigreturn(2).
		 */
		if (tf->tf_flags & FRAME_SYSCALL)
			KASSERT(tf->tf_scratch.gr15 == SYS_sigreturn, ("foo"));
		tf->tf_scratch = mc->mc_scratch;
		tf->tf_scratch_fp = mc->mc_scratch_fp;
		if (mc->mc_flags & _MC_FLAGS_HIGHFP_VALID)
			td->td_pcb->pcb_high_fp = mc->mc_high_fp;
	} else {
		KASSERT((tf->tf_flags & FRAME_SYSCALL) != 0, ("foo"));
		if ((mc->mc_flags & _MC_FLAGS_SCRATCH_VALID) == 0) {
			s.cfm = tf->tf_special.cfm;
			s.iip = tf->tf_special.iip;
			tf->tf_scratch.gr15 = 0;	/* Clear syscall nr. */
			if (mc->mc_flags & _MC_FLAGS_RETURN_VALID) {
				tf->tf_scratch.gr8 = mc->mc_scratch.gr8;
				tf->tf_scratch.gr9 = mc->mc_scratch.gr9;
				tf->tf_scratch.gr10 = mc->mc_scratch.gr10;
				tf->tf_scratch.gr11 = mc->mc_scratch.gr11;
			}
		} else
			tf->tf_scratch = mc->mc_scratch;
	}
	tf->tf_special = s;
	restore_callee_saved(&mc->mc_preserved);
	restore_callee_saved_fp(&mc->mc_preserved_fp);

	if (mc->mc_flags & _MC_FLAGS_KSE_SET_MBOX)
		suword((caddr_t)mc->mc_special.ifa, mc->mc_special.isr);

	return (0);
}

/*
 * Clear registers on exec.
 */
void
exec_setregs(struct thread *td, u_long entry, u_long stack, u_long ps_strings)
{
	struct trapframe *tf;
	uint64_t *ksttop, *kst;

	tf = td->td_frame;
	ksttop = (uint64_t*)(td->td_kstack + tf->tf_special.ndirty +
	    (tf->tf_special.bspstore & 0x1ffUL));

	/*
	 * We can ignore up to 8KB of dirty registers by masking off the
	 * lower 13 bits in exception_restore() or epc_syscall(). This
	 * should be enough for a couple of years, but if there are more
	 * than 8KB of dirty registers, we lose track of the bottom of
	 * the kernel stack. The solution is to copy the active part of
	 * the kernel stack down 1 page (or 2, but not more than that)
	 * so that we always have less than 8KB of dirty registers.
	 */
	KASSERT((tf->tf_special.ndirty & ~PAGE_MASK) == 0,
	    ("Whoa there! We have more than 8KB of dirty registers!"));

	bzero(&tf->tf_special, sizeof(tf->tf_special));
	if ((tf->tf_flags & FRAME_SYSCALL) == 0) {	/* break syscalls. */
		bzero(&tf->tf_scratch, sizeof(tf->tf_scratch));
		bzero(&tf->tf_scratch_fp, sizeof(tf->tf_scratch_fp));
		tf->tf_special.cfm = (1UL<<63) | (3UL<<7) | 3UL;
		tf->tf_special.bspstore = IA64_BACKINGSTORE;
		/*
		 * Copy the arguments onto the kernel register stack so that
		 * they get loaded by the loadrs instruction. Skip over the
		 * NaT collection points.
		 */
		kst = ksttop - 1;
		if (((uintptr_t)kst & 0x1ff) == 0x1f8)
			*kst-- = 0;
		*kst-- = 0;
		if (((uintptr_t)kst & 0x1ff) == 0x1f8)
			*kst-- = 0;
		*kst-- = ps_strings;
		if (((uintptr_t)kst & 0x1ff) == 0x1f8)
			*kst-- = 0;
		*kst = stack;
		tf->tf_special.ndirty = (ksttop - kst) << 3;
	} else {				/* epc syscalls (default). */
		tf->tf_special.cfm = (3UL<<62) | (3UL<<7) | 3UL;
		tf->tf_special.bspstore = IA64_BACKINGSTORE + 24;
		/*
		 * Write values for out0, out1 and out2 to the user's backing
		 * store and arrange for them to be restored into the user's
		 * initial register frame.
		 * Assumes that (bspstore & 0x1f8) < 0x1e0.
		 */
		suword((caddr_t)tf->tf_special.bspstore - 24, stack);
		suword((caddr_t)tf->tf_special.bspstore - 16, ps_strings);
		suword((caddr_t)tf->tf_special.bspstore -  8, 0);
	}

	tf->tf_special.iip = entry;
	tf->tf_special.sp = (stack & ~15) - 16;
	tf->tf_special.rsc = 0xf;
	tf->tf_special.fpsr = IA64_FPSR_DEFAULT;
	tf->tf_special.psr = IA64_PSR_IC | IA64_PSR_I | IA64_PSR_IT |
	    IA64_PSR_DT | IA64_PSR_RT | IA64_PSR_DFH | IA64_PSR_BN |
	    IA64_PSR_CPL_USER;
}

int
ptrace_set_pc(struct thread *td, unsigned long addr)
{
	uint64_t slot;

	switch (addr & 0xFUL) {
	case 0:
		slot = IA64_PSR_RI_0;
		break;
	case 1:
		/* XXX we need to deal with MLX bundles here */
		slot = IA64_PSR_RI_1;
		break;
	case 2:
		slot = IA64_PSR_RI_2;
		break;
	default:
		return (EINVAL);
	}

	td->td_frame->tf_special.iip = addr & ~0x0FULL;
	td->td_frame->tf_special.psr =
	    (td->td_frame->tf_special.psr & ~IA64_PSR_RI) | slot;
	return (0);
}

int
ptrace_single_step(struct thread *td)
{

	td->td_frame->tf_special.psr |= IA64_PSR_SS;
	return (0);
}

int
fill_regs(struct thread *td, struct reg *regs)
{
	struct trapframe *tf;

	tf = td->td_frame;
	regs->r_special = tf->tf_special;
	regs->r_scratch = tf->tf_scratch;
	save_callee_saved(&regs->r_preserved);
	return (0);
}

int
set_regs(struct thread *td, struct reg *regs)
{
	struct trapframe *tf;

	tf = td->td_frame;
	tf->tf_special = regs->r_special;
	tf->tf_scratch = regs->r_scratch;
	restore_callee_saved(&regs->r_preserved);
	return (0);
}

int
fill_dbregs(struct thread *td, struct dbreg *dbregs)
{

	return (ENOSYS);
}

int
set_dbregs(struct thread *td, struct dbreg *dbregs)
{

	return (ENOSYS);
}

int
fill_fpregs(struct thread *td, struct fpreg *fpregs)
{
	struct trapframe *frame = td->td_frame;
	struct pcb *pcb = td->td_pcb;

	/* Save the high FP registers. */
	ia64_highfp_save(td);

	fpregs->fpr_scratch = frame->tf_scratch_fp;
	save_callee_saved_fp(&fpregs->fpr_preserved);
	fpregs->fpr_high = pcb->pcb_high_fp;
	return (0);
}

int
set_fpregs(struct thread *td, struct fpreg *fpregs)
{
	struct trapframe *frame = td->td_frame;
	struct pcb *pcb = td->td_pcb;

	/* Throw away the high FP registers (should be redundant). */
	ia64_highfp_drop(td);

	frame->tf_scratch_fp = fpregs->fpr_scratch;
	restore_callee_saved_fp(&fpregs->fpr_preserved);
	pcb->pcb_high_fp = fpregs->fpr_high;
	return (0);
}

/*
 * High FP register functions.
 * XXX no synchronization yet.
 */

int
ia64_highfp_drop(struct thread *td)
{
	struct pcb *pcb;
	struct pcpu *cpu;
	struct thread *thr;

	pcb = td->td_pcb;
	cpu = pcb->pcb_fpcpu;
	if (cpu == NULL)
		return (0);
	pcb->pcb_fpcpu = NULL;
	thr = cpu->pc_fpcurthread;
	cpu->pc_fpcurthread = NULL;

	/* Post-mortem sanity checking. */
	KASSERT(thr == td, ("Inconsistent high FP state"));
	return (1);
}

int
ia64_highfp_save(struct thread *td)
{
	struct pcb *pcb;
	struct pcpu *cpu;
	struct thread *thr;

	/* Don't save if the high FP registers weren't modified. */
	if ((td->td_frame->tf_special.psr & IA64_PSR_MFH) == 0)
		return (ia64_highfp_drop(td));

	pcb = td->td_pcb;
	cpu = pcb->pcb_fpcpu;
	if (cpu == NULL)
		return (0);
#ifdef SMP
	if (cpu != pcpup) {
		ipi_send(cpu->pc_lid, IPI_HIGH_FP);
		while (pcb->pcb_fpcpu != cpu)
			DELAY(100);
		return (1);
	}
#endif
	save_high_fp(&pcb->pcb_high_fp);
	pcb->pcb_fpcpu = NULL;
	thr = cpu->pc_fpcurthread;
	cpu->pc_fpcurthread = NULL;

	/* Post-mortem sanity cxhecking. */
	KASSERT(thr == td, ("Inconsistent high FP state"));
	return (1);
}

#ifndef DDB
void
Debugger(const char *msg)
{
	printf("Debugger(\"%s\") called.\n", msg);
}
#endif /* no DDB */

int
sysbeep(int pitch, int period)
{
	return (ENODEV);
}
