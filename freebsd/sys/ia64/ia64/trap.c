/* From: src/sys/alpha/alpha/trap.c,v 1.33 */
/* $NetBSD: trap.c,v 1.31 1998/03/26 02:21:46 thorpej Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_ddb.h"
#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ktr.h>
#include <sys/sysproto.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/smp.h>
#include <sys/vmmeter.h>
#include <sys/sysent.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/pioctl.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>
#include <vm/vm_extern.h>
#include <vm/vm_param.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/md_var.h>
#include <machine/reg.h>
#include <machine/pal.h>
#include <machine/fpu.h>
#include <machine/efi.h>
#ifdef SMP
#include <machine/smp.h>
#endif

#ifdef KTRACE
#include <sys/uio.h>
#include <sys/ktrace.h>
#endif

#ifdef DDB
#include <ddb/ddb.h>
#endif

static int print_usertrap = 0;
SYSCTL_INT(_machdep, OID_AUTO, print_usertrap,
    CTLFLAG_RW, &print_usertrap, 0, "");

static void break_syscall(struct trapframe *tf);
static void ia32_syscall(struct trapframe *framep);

/*
 * EFI-Provided FPSWA interface (Floating Point SoftWare Assist)
 */

/* The function entry address */
extern FPSWA_INTERFACE *fpswa_interface;

/* Copy of the faulting instruction bundle */
typedef struct {
	u_int64_t	bundle_low64;
	u_int64_t	bundle_high64;
} FPSWA_BUNDLE;

/*
 * The fp state descriptor... tell FPSWA where the "true" copy is.
 * We save some registers in the trapframe, so we have to point some of
 * these there.  The rest of the registers are "live"
 */
typedef struct {
	u_int64_t	bitmask_low64;			/* f63 - f2 */
	u_int64_t	bitmask_high64;			/* f127 - f64 */
	union _ia64_fpreg *fp_low_preserved;		/* f2 - f5 */
	union _ia64_fpreg *fp_low_volatile;		/* f6 - f15 */
	union _ia64_fpreg *fp_high_preserved;		/* f16 - f31 */
	union _ia64_fpreg *fp_high_volatile;		/* f32 - f127 */
} FP_STATE;

#ifdef WITNESS
extern char *syscallnames[];
#endif

static const char *ia64_vector_names[] = {
	"VHPT Translation",			/* 0 */
	"Instruction TLB",			/* 1 */
	"Data TLB",				/* 2 */
	"Alternate Instruction TLB",		/* 3 */
	"Alternate Data TLB",			/* 4 */
	"Data Nested TLB",			/* 5 */
	"Instruction Key Miss",			/* 6 */
	"Data Key Miss",			/* 7 */
	"Dirty-Bit",				/* 8 */
	"Instruction Access-Bit",		/* 9 */
	"Data Access-Bit",			/* 10 */
	"Break Instruction",			/* 11 */
	"External Interrupt",			/* 12 */
	"Reserved 13",				/* 13 */
	"Reserved 14",				/* 14 */
	"Reserved 15",				/* 15 */
	"Reserved 16",				/* 16 */
	"Reserved 17",				/* 17 */
	"Reserved 18",				/* 18 */
	"Reserved 19",				/* 19 */
	"Page Not Present",			/* 20 */
	"Key Permission",			/* 21 */
	"Instruction Access Rights",		/* 22 */
	"Data Access Rights",			/* 23 */
	"General Exception",			/* 24 */
	"Disabled FP-Register",			/* 25 */
	"NaT Consumption",			/* 26 */
	"Speculation",				/* 27 */
	"Reserved 28",				/* 28 */
	"Debug",				/* 29 */
	"Unaligned Reference",			/* 30 */
	"Unsupported Data Reference",		/* 31 */
	"Floating-point Fault",			/* 32 */
	"Floating-point Trap",			/* 33 */
	"Lower-Privilege Transfer Trap",	/* 34 */
	"Taken Branch Trap",			/* 35 */
	"Single Step Trap",			/* 36 */
	"Reserved 37",				/* 37 */
	"Reserved 38",				/* 38 */
	"Reserved 39",				/* 39 */
	"Reserved 40",				/* 40 */
	"Reserved 41",				/* 41 */
	"Reserved 42",				/* 42 */
	"Reserved 43",				/* 43 */
	"Reserved 44",				/* 44 */
	"IA-32 Exception",			/* 45 */
	"IA-32 Intercept",			/* 46 */
	"IA-32 Interrupt",			/* 47 */
	"Reserved 48",				/* 48 */
	"Reserved 49",				/* 49 */
	"Reserved 50",				/* 50 */
	"Reserved 51",				/* 51 */
	"Reserved 52",				/* 52 */
	"Reserved 53",				/* 53 */
	"Reserved 54",				/* 54 */
	"Reserved 55",				/* 55 */
	"Reserved 56",				/* 56 */
	"Reserved 57",				/* 57 */
	"Reserved 58",				/* 58 */
	"Reserved 59",				/* 59 */
	"Reserved 60",				/* 60 */
	"Reserved 61",				/* 61 */
	"Reserved 62",				/* 62 */
	"Reserved 63",				/* 63 */
	"Reserved 64",				/* 64 */
	"Reserved 65",				/* 65 */
	"Reserved 66",				/* 66 */
	"Reserved 67",				/* 67 */
};

struct bitname {
	u_int64_t mask;
	const char* name;
};

static void
printbits(u_int64_t mask, struct bitname *bn, int count)
{
	int i, first = 1;
	u_int64_t bit;

	for (i = 0; i < count; i++) {
		/*
		 * Handle fields wider than one bit.
		 */
		bit = bn[i].mask & ~(bn[i].mask - 1);
		if (bn[i].mask > bit) {
			if (first)
				first = 0;
			else
				printf(",");
			printf("%s=%ld", bn[i].name,
			       (mask & bn[i].mask) / bit);
		} else if (mask & bit) {
			if (first)
				first = 0;
			else
				printf(",");
			printf("%s", bn[i].name);
		}
	}
}

struct bitname psr_bits[] = {
	{IA64_PSR_BE,	"be"},
	{IA64_PSR_UP,	"up"},
	{IA64_PSR_AC,	"ac"},
	{IA64_PSR_MFL,	"mfl"},
	{IA64_PSR_MFH,	"mfh"},
	{IA64_PSR_IC,	"ic"},
	{IA64_PSR_I,	"i"},
	{IA64_PSR_PK,	"pk"},
	{IA64_PSR_DT,	"dt"},
	{IA64_PSR_DFL,	"dfl"},
	{IA64_PSR_DFH,	"dfh"},
	{IA64_PSR_SP,	"sp"},
	{IA64_PSR_PP,	"pp"},
	{IA64_PSR_DI,	"di"},
	{IA64_PSR_SI,	"si"},
	{IA64_PSR_DB,	"db"},
	{IA64_PSR_LP,	"lp"},
	{IA64_PSR_TB,	"tb"},
	{IA64_PSR_RT,	"rt"},
	{IA64_PSR_CPL,	"cpl"},
	{IA64_PSR_IS,	"is"},
	{IA64_PSR_MC,	"mc"},
	{IA64_PSR_IT,	"it"},
	{IA64_PSR_ID,	"id"},
	{IA64_PSR_DA,	"da"},
	{IA64_PSR_DD,	"dd"},
	{IA64_PSR_SS,	"ss"},
	{IA64_PSR_RI,	"ri"},
	{IA64_PSR_ED,	"ed"},
	{IA64_PSR_BN,	"bn"},
	{IA64_PSR_IA,	"ia"},
};

static void
printpsr(u_int64_t psr)
{
	printbits(psr, psr_bits, sizeof(psr_bits)/sizeof(psr_bits[0]));
}

struct bitname isr_bits[] = {
	{IA64_ISR_CODE,	"code"},
	{IA64_ISR_VECTOR, "vector"},
	{IA64_ISR_X,	"x"},
	{IA64_ISR_W,	"w"},
	{IA64_ISR_R,	"r"},
	{IA64_ISR_NA,	"na"},
	{IA64_ISR_SP,	"sp"},
	{IA64_ISR_RS,	"rs"},
	{IA64_ISR_IR,	"ir"},
	{IA64_ISR_NI,	"ni"},
	{IA64_ISR_SO,	"so"},
	{IA64_ISR_EI,	"ei"},
	{IA64_ISR_ED,	"ed"},
};

static void printisr(u_int64_t isr)
{
	printbits(isr, isr_bits, sizeof(isr_bits)/sizeof(isr_bits[0]));
}

static void
printtrap(int vector, struct trapframe *framep, int isfatal, int user)
{
	printf("\n");
	printf("%s %s trap (cpu %d):\n", isfatal? "fatal" : "handled",
	       user ? "user" : "kernel", PCPU_GET(cpuid));
	printf("\n");
	printf("    trap vector = 0x%x (%s)\n",
	       vector, ia64_vector_names[vector]);
	printf("    cr.iip      = 0x%lx\n", framep->tf_special.iip);
	printf("    cr.ipsr     = 0x%lx (", framep->tf_special.psr);
	printpsr(framep->tf_special.psr);
	printf(")\n");
	printf("    cr.isr      = 0x%lx (", framep->tf_special.isr);
	printisr(framep->tf_special.isr);
	printf(")\n");
	printf("    cr.ifa      = 0x%lx\n", framep->tf_special.ifa);
	if (framep->tf_special.psr & IA64_PSR_IS) {
		printf("    ar.cflg     = 0x%lx\n", ia64_get_cflg());
		printf("    ar.csd      = 0x%lx\n", ia64_get_csd());
		printf("    ar.ssd      = 0x%lx\n", ia64_get_ssd());
	}
	printf("    curthread   = %p\n", curthread);
	if (curthread != NULL)
		printf("        pid = %d, comm = %s\n",
		       curthread->td_proc->p_pid, curthread->td_proc->p_comm);
	printf("\n");
}

static void
trap_panic(int vector, struct trapframe *tf)
{

	printtrap(vector, tf, 1, TRAPF_USERMODE(tf));
#ifdef DDB
	kdb_trap(vector, tf);
#endif
	panic("trap");
}

/*
 *
 */
int
do_ast(struct trapframe *tf)
{

	disable_intr();
	while (curthread->td_flags & (TDF_ASTPENDING|TDF_NEEDRESCHED)) {
		enable_intr();
		ast(tf);
		disable_intr();
	}
	/*
	 * Keep interrupts disabled. We return r10 as a favor to the EPC
	 * syscall code so that it can quicky determine if the syscall
	 * needs to be restarted or not.
	 */
	return (tf->tf_scratch.gr10);
}

/*
 * Trap is called from exception.s to handle most types of processor traps.
 */
/*ARGSUSED*/
void
trap(int vector, struct trapframe *framep)
{
	struct proc *p;
	struct thread *td;
	u_int64_t ucode;
	int error, sig, user;
	u_int sticks;

	user = TRAPF_USERMODE(framep) ? 1 : 0;

	/* Sanitize the FP state in case the user has trashed it. */
	ia64_set_fpsr(IA64_FPSR_DEFAULT);

	atomic_add_int(&cnt.v_trap, 1);

	td = curthread;
	p = td->td_proc;
	ucode = 0;

	if (user) {
		sticks = td->td_sticks;
		td->td_frame = framep;
		if (td->td_ucred != p->p_ucred)
			cred_update_thread(td);
	} else {
		sticks = 0;		/* XXX bogus -Wuninitialized warning */
		KASSERT(cold || td->td_ucred != NULL,
		    ("kernel trap doesn't have ucred"));
	}

	sig = 0;
	switch (vector) {
	case IA64_VEC_VHPT:
		/*
		 * This one is tricky. We should hardwire the VHPT, but
		 * don't at this time. I think we're mostly lucky that
		 * the VHPT is mapped.
		 */
		trap_panic(vector, framep);
		break;

	case IA64_VEC_ITLB:
	case IA64_VEC_DTLB:
	case IA64_VEC_EXT_INTR:
		/* We never call trap() with these vectors. */
		trap_panic(vector, framep);
		break;

	case IA64_VEC_ALT_ITLB:
	case IA64_VEC_ALT_DTLB:
		/*
		 * These should never happen, because regions 0-4 use the
		 * VHPT. If we get one of these it means we didn't program
		 * the region registers correctly.
		 */
		trap_panic(vector, framep);
		break;

	case IA64_VEC_NESTED_DTLB:
		/*
		 * We never call trap() with this vector. We may want to
		 * do that in the future in case the nested TLB handler
		 * could not find the translation it needs. In that case
		 * we could switch to a special (hardwired) stack and
		 * come here to produce a nice panic().
		 */
		trap_panic(vector, framep);
		break;

	case IA64_VEC_IKEY_MISS:
	case IA64_VEC_DKEY_MISS:
	case IA64_VEC_KEY_PERMISSION:
		/*
		 * We don't use protection keys, so we should never get
		 * these faults.
		 */
		trap_panic(vector, framep);
		break;

	case IA64_VEC_DIRTY_BIT:
	case IA64_VEC_INST_ACCESS:
	case IA64_VEC_DATA_ACCESS:
		/*
		 * We get here if we read or write to a page of which the
		 * PTE does not have the access bit or dirty bit set and
		 * we can not find the PTE in our datastructures. This
		 * either means we have a stale PTE in the TLB, or we lost
		 * the PTE in our datastructures.
		 */
		trap_panic(vector, framep);
		break;

	case IA64_VEC_BREAK:
		if (user) {
			if (framep->tf_special.ifa == 0x100000) {
				break_syscall(framep);
				return;		/* do_ast() already called. */
			} else if (framep->tf_special.ifa == 0x180000) {
				mcontext_t mc;

				error = copyin((void*)framep->tf_scratch.gr8,
				    &mc, sizeof(mc));
				if (!error) {
					set_mcontext(td, &mc);
					return;	/* Don't call do_ast()!!! */
				}
				ucode = framep->tf_scratch.gr8;
				sig = SIGSEGV;
			} else {
				framep->tf_special.psr &= ~IA64_PSR_SS;
				sig = SIGTRAP;
			}
		} else {
#ifdef DDB
			if (kdb_trap(vector, framep))
				return;
			panic("trap");
#else
			trap_panic(vector, framep);
#endif
		}
		break;

	case IA64_VEC_PAGE_NOT_PRESENT:
	case IA64_VEC_INST_ACCESS_RIGHTS:
	case IA64_VEC_DATA_ACCESS_RIGHTS: {
		vm_offset_t va;
		struct vmspace *vm;
		vm_map_t map;
		vm_prot_t ftype;
		int rv;

		rv = 0;
		va = trunc_page(framep->tf_special.ifa);

		if (va >= VM_MAX_ADDRESS) {
			/*
			 * Don't allow user-mode faults for kernel virtual
			 * addresses, including the gateway page.
			 */
			if (user)
				goto no_fault_in;
			map = kernel_map;
		} else {
			vm = (p != NULL) ? p->p_vmspace : NULL;
			if (vm == NULL)
				goto no_fault_in;
			map = &vm->vm_map;
		}

		if (framep->tf_special.isr & IA64_ISR_X)
			ftype = VM_PROT_EXECUTE;
		else if (framep->tf_special.isr & IA64_ISR_W)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

		if (map != kernel_map) {
			/*
			 * Keep swapout from messing with us during this
			 * critical time.
			 */
			PROC_LOCK(p);
			++p->p_lock;
			PROC_UNLOCK(p);

			/* Fault in the user page: */
			rv = vm_fault(map, va, ftype, (ftype & VM_PROT_WRITE)
			    ? VM_FAULT_DIRTY : VM_FAULT_NORMAL);

			PROC_LOCK(p);
			--p->p_lock;
			PROC_UNLOCK(p);
		} else {
			/*
			 * Don't have to worry about process locking or
			 * stacks in the kernel.
			 */
			rv = vm_fault(map, va, ftype, VM_FAULT_NORMAL);
		}

		if (rv == KERN_SUCCESS)
			goto out;

	no_fault_in:
		if (!user) {
			/* Check for copyin/copyout fault. */
			if (td != NULL && td->td_pcb->pcb_onfault != 0) {
				framep->tf_special.iip =
				    td->td_pcb->pcb_onfault;
				framep->tf_special.psr &= ~IA64_PSR_RI;
				td->td_pcb->pcb_onfault = 0;
				goto out;
			}
			trap_panic(vector, framep);
		}
		ucode = va;
		sig = (rv == KERN_PROTECTION_FAILURE) ? SIGBUS : SIGSEGV;
		break;
	}

	case IA64_VEC_GENERAL_EXCEPTION:
	case IA64_VEC_NAT_CONSUMPTION:
	case IA64_VEC_SPECULATION:
	case IA64_VEC_UNSUPP_DATA_REFERENCE:
	case IA64_VEC_LOWER_PRIVILEGE_TRANSFER:
		if (user) {
			ucode = vector;
			sig = SIGILL;
		} else
			trap_panic(vector, framep);
		break;

	case IA64_VEC_DISABLED_FP: {
		struct pcpu *pcpu;
		struct pcb *pcb;
		struct thread *thr;

		/* Always fatal in kernel. Should never happen. */
		if (!user)
			trap_panic(vector, framep);

		critical_enter();
		thr = PCPU_GET(fpcurthread);
		if (thr == td) {
			/*
			 * Short-circuit handling the trap when this CPU
			 * already holds the high FP registers for this
			 * thread.  We really shouldn't get the trap in the
			 * first place, but since it's only a performance
			 * issue and not a correctness issue, we emit a
			 * message for now, enable the high FP registers and
			 * return.
			 */
			printf("XXX: bogusly disabled high FP regs\n");
			framep->tf_special.psr &= ~IA64_PSR_DFH;
			critical_exit();
			goto out;
		} else if (thr != NULL) {
			pcb = thr->td_pcb;
			save_high_fp(&pcb->pcb_high_fp);
			pcb->pcb_fpcpu = NULL;
			PCPU_SET(fpcurthread, NULL);
			thr = NULL;
		}

		pcb = td->td_pcb;
		pcpu = pcb->pcb_fpcpu;

#ifdef SMP
		if (pcpu != NULL) {
			ipi_send(pcpu->pc_lid, IPI_HIGH_FP);
			critical_exit();
			while (pcb->pcb_fpcpu != pcpu)
				DELAY(100);
			critical_enter();
			pcpu = pcb->pcb_fpcpu;
			thr = PCPU_GET(fpcurthread);
		}
#endif

		if (thr == NULL && pcpu == NULL) {
			restore_high_fp(&pcb->pcb_high_fp);
			PCPU_SET(fpcurthread, td);
			pcb->pcb_fpcpu = pcpup;
			framep->tf_special.psr &= ~IA64_PSR_MFH;
			framep->tf_special.psr &= ~IA64_PSR_DFH;
		}

		critical_exit();
		goto out;
	}

	case IA64_VEC_DEBUG:
	case IA64_VEC_TAKEN_BRANCH_TRAP:
	case IA64_VEC_SINGLE_STEP_TRAP:
		framep->tf_special.psr &= ~IA64_PSR_SS;
		if (!user) {
#ifdef DDB
			if (kdb_trap(vector, framep))
				return;
			panic("trap");
#else
			trap_panic(vector, framep);
#endif
		}
		sig = SIGTRAP;
		break;

	case IA64_VEC_UNALIGNED_REFERENCE:
		/*
		 * If user-land, do whatever fixups, printing, and
		 * signalling is appropriate (based on system-wide
		 * and per-process unaligned-access-handling flags).
		 */
		if (user) {
			sig = unaligned_fixup(framep, td);
			if (sig == 0)
				goto out;
			ucode = framep->tf_special.ifa;	/* VA */
		} else
			trap_panic(vector, framep);
		break;

	case IA64_VEC_FLOATING_POINT_FAULT:
	case IA64_VEC_FLOATING_POINT_TRAP: {
		FP_STATE fp_state;
		FPSWA_RET fpswa_ret;
		FPSWA_BUNDLE bundle;

		/* Always fatal in kernel. Should never happen. */
		if (!user)
			trap_panic(vector, framep);

		if (fpswa_interface == NULL) {
			sig = SIGFPE;
			ucode = 0;
			break;
		}

		error = copyin((void *)(framep->tf_special.iip), &bundle, 16);
		if (error) {
			sig = SIGBUS;	/* EFAULT, basically */
			ucode = 0;	/* exception summary */
			break;
		}

		/* f6-f15 are saved in exception_save */
		fp_state.bitmask_low64 = 0xffc0;	/* bits 6 - 15 */
		fp_state.bitmask_high64 = 0x0;
		fp_state.fp_low_preserved = NULL;
		fp_state.fp_low_volatile = &framep->tf_scratch_fp.fr6;
		fp_state.fp_high_preserved = NULL;
		fp_state.fp_high_volatile = NULL;

		/*
		 * We have the high FP registers disabled while in the
		 * kernel. Enable them for the FPSWA handler only.
		 */
		ia64_enable_highfp();

		/* The docs are unclear.  Is Fpswa reentrant? */
		fpswa_ret = fpswa_interface->Fpswa(1, &bundle,
		    &framep->tf_special.psr, &framep->tf_special.fpsr,
		    &framep->tf_special.isr, &framep->tf_special.pr,
		    &framep->tf_special.cfm, &fp_state);

		ia64_disable_highfp();

		if (fpswa_ret.status == 0) {
			/* fixed.  update ipsr and iip to next insn */
			int ei;

			ei = (framep->tf_special.isr >> 41) & 0x03;
			if (ei == 0) {		/* no template for this case */
				framep->tf_special.psr &= ~IA64_ISR_EI;
				framep->tf_special.psr |= IA64_ISR_EI_1;
			} else if (ei == 1) {	/* MFI or MFB */
				framep->tf_special.psr &= ~IA64_ISR_EI;
				framep->tf_special.psr |= IA64_ISR_EI_2;
			} else if (ei == 2) {	/* MMF */
				framep->tf_special.psr &= ~IA64_ISR_EI;
				framep->tf_special.iip += 0x10;
			}
			goto out;
		} else if (fpswa_ret.status == -1) {
			printf("FATAL: FPSWA err1 %lx, err2 %lx, err3 %lx\n",
			    fpswa_ret.err1, fpswa_ret.err2, fpswa_ret.err3);
			panic("fpswa fatal error on fp fault");
		} else if (fpswa_ret.status > 0) {
#if 0
			if (fpswa_ret.status & 1) {
				/*
				 * New exception needs to be raised.
				 * If set then the following bits also apply:
				 * & 2 -> fault was converted to a trap
				 * & 4 -> SIMD caused the exception
				 */
				sig = SIGFPE;
				ucode = 0;		/* exception summary */
				break;
			}
#endif
			sig = SIGFPE;
			ucode = 0;			/* exception summary */
			break;
		} else
			panic("bad fpswa return code %lx", fpswa_ret.status);
	}

	case IA64_VEC_IA32_EXCEPTION:
		switch ((framep->tf_special.isr >> 16) & 0xffff) {
		case IA32_EXCEPTION_DIVIDE:
			ucode = FPE_INTDIV;
			sig = SIGFPE;
			break;
		case IA32_EXCEPTION_DEBUG:
		case IA32_EXCEPTION_BREAK:
			sig = SIGTRAP;
			break;
		case IA32_EXCEPTION_OVERFLOW:
			ucode = FPE_INTOVF;
			sig = SIGFPE;
			break;
		case IA32_EXCEPTION_BOUND:
			ucode = FPE_FLTSUB;
			sig = SIGFPE;
			break;
		case IA32_EXCEPTION_DNA:
			ucode = 0;
			sig = SIGFPE;
			break;
		case IA32_EXCEPTION_NOT_PRESENT:
		case IA32_EXCEPTION_STACK_FAULT:
		case IA32_EXCEPTION_GPFAULT:
			ucode = (framep->tf_special.isr & 0xffff) +
			    BUS_SEGM_FAULT;
			sig = SIGBUS;
			break;
		case IA32_EXCEPTION_FPERROR:
			ucode = 0;	/* XXX */
			sig = SIGFPE;
			break;
		case IA32_EXCEPTION_ALIGNMENT_CHECK:
			ucode = framep->tf_special.ifa;	/* VA */
			sig = SIGBUS;
			break;
		case IA32_EXCEPTION_STREAMING_SIMD:
			ucode = 0; /* XXX */
			sig = SIGFPE;
			break;
		default:
			trap_panic(vector, framep);
			break;
		}
		break;

	case IA64_VEC_IA32_INTERCEPT:
		/* XXX Maybe need to emulate ia32 instruction. */
		trap_panic(vector, framep);

	case IA64_VEC_IA32_INTERRUPT:
		/* INT n instruction - probably a syscall. */
		if (((framep->tf_special.isr >> 16) & 0xffff) == 0x80) {
			ia32_syscall(framep);
			goto out;
		}
		ucode = (framep->tf_special.isr >> 16) & 0xffff;
		sig = SIGILL;
		break;

	default:
		/* Reserved vectors get here. Should never happen of course. */
		trap_panic(vector, framep);
		break;
	}

	KASSERT(sig != 0, ("foo"));

	if (print_usertrap)
		printtrap(vector, framep, 1, user);

	trapsignal(td, sig, ucode);

out:
	if (user) {
		userret(td, framep, sticks);
		mtx_assert(&Giant, MA_NOTOWNED);
#ifdef DIAGNOSTIC
		cred_free_thread(td);
#endif
		do_ast(framep);
	}
	return;
}


/*
 * Handle break instruction based system calls.
 */
void
break_syscall(struct trapframe *tf)
{
	uint64_t *bsp, *tfp;
	uint64_t iip, psr;
	int error, nargs;

	/* Save address of break instruction. */
	iip = tf->tf_special.iip;
	psr = tf->tf_special.psr;

	/* Advance to the next instruction. */
	tf->tf_special.psr += IA64_PSR_RI_1;
	if ((tf->tf_special.psr & IA64_PSR_RI) > IA64_PSR_RI_2) {
		tf->tf_special.iip += 16;
		tf->tf_special.psr &= ~IA64_PSR_RI;
	}

	/*
	 * Copy the arguments on the register stack into the trapframe
	 * to avoid having interleaved NaT collections.
	 */
	tfp = &tf->tf_scratch.gr16;
	nargs = tf->tf_special.cfm & 0x7f;
	bsp = (uint64_t*)(curthread->td_kstack + tf->tf_special.ndirty +
	    (tf->tf_special.bspstore & 0x1ffUL));
	bsp -= (((uintptr_t)bsp & 0x1ff) < (nargs << 3)) ? (nargs + 1): nargs;
	while (nargs--) {
		*tfp++ = *bsp++;
		if (((uintptr_t)bsp & 0x1ff) == 0x1f8)
			bsp++;
	}
	error = syscall(tf);
	if (error == ERESTART) {
		tf->tf_special.iip = iip;
		tf->tf_special.psr = psr;
	}

	do_ast(tf);
}

/*
 * Process a system call.
 *
 * See syscall.s for details as to how we get here. In order to support
 * the ERESTART case, we return the error to our caller. They deal with
 * the hairy details.
 */
int
syscall(struct trapframe *tf)
{
	struct sysent *callp;
	struct proc *p;
	struct thread *td;
	u_int64_t *args;
	int code, error;
	u_int sticks;

	code = tf->tf_scratch.gr15;
	args = &tf->tf_scratch.gr16;

	atomic_add_int(&cnt.v_syscall, 1);

	td = curthread;
	p = td->td_proc;

	td->td_frame = tf;
	sticks = td->td_sticks;
	if (td->td_ucred != p->p_ucred)
		cred_update_thread(td);
	if (p->p_flag & P_SA)
		thread_user_enter(p, td);

	if (p->p_sysent->sv_prepsyscall) {
		/* (*p->p_sysent->sv_prepsyscall)(tf, args, &code, &params); */
		panic("prepsyscall");
	} else {
		/*
		 * syscall() and __syscall() are handled the same on
		 * the ia64, as everything is 64-bit aligned, anyway.
		 */
		if (code == SYS_syscall || code == SYS___syscall) {
			/*
			 * Code is first argument, followed by actual args.
			 */
			code = args[0];
			args++;
		}
	}

 	if (p->p_sysent->sv_mask)
 		code &= p->p_sysent->sv_mask;

 	if (code >= p->p_sysent->sv_size)
 		callp = &p->p_sysent->sv_table[0];
  	else
 		callp = &p->p_sysent->sv_table[code];

	/*
	 * Try to run the syscall without Giant if the syscall is MP safe.
	 */
	if ((callp->sy_narg & SYF_MPSAFE) == 0)
		mtx_lock(&Giant);
#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSCALL))
		ktrsyscall(code, (callp->sy_narg & SYF_ARGMASK), args);
#endif

	td->td_retval[0] = 0;
	td->td_retval[1] = 0;
	tf->tf_scratch.gr10 = EJUSTRETURN;

	STOPEVENT(p, S_SCE, (callp->sy_narg & SYF_ARGMASK));

	PTRACESTOP_SC(p, td, S_PT_SCE);

	error = (*callp->sy_call)(td, args);

	if (error != EJUSTRETURN) {
		/*
		 * Save the "raw" error code in r10. We use this to handle
		 * syscall restarts (see do_ast()).
		 */
		tf->tf_scratch.gr10 = error;
		if (error == 0) {
			tf->tf_scratch.gr8 = td->td_retval[0];
			tf->tf_scratch.gr9 = td->td_retval[1];
		} else if (error != ERESTART) {
			if (error < p->p_sysent->sv_errsize)
				error = p->p_sysent->sv_errtbl[error];
			/*
			 * Translated error codes are returned in r8. User
			 * processes use the translated error code.
			 */
			tf->tf_scratch.gr8 = error;
		}
	}

	/*
	 * Release Giant if we had to get it.
	 */
	if ((callp->sy_narg & SYF_MPSAFE) == 0)
		mtx_unlock(&Giant);

	userret(td, tf, sticks);

#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSRET))
		ktrsysret(code, error, td->td_retval[0]);
#endif

	/*
	 * This works because errno is findable through the
	 * register set.  If we ever support an emulation where this
	 * is not the case, this code will need to be revisited.
	 */
	STOPEVENT(p, S_SCX, code);

	PTRACESTOP_SC(p, td, S_PT_SCX);

#ifdef DIAGNOSTIC
	cred_free_thread(td);
#endif

	WITNESS_WARN(WARN_PANIC, NULL, "System call %s returning",
	    (code >= 0 && code < SYS_MAXSYSCALL) ? syscallnames[code] : "???");
	mtx_assert(&sched_lock, MA_NOTOWNED);
	mtx_assert(&Giant, MA_NOTOWNED);

	return (error);
}

#include <i386/include/psl.h>

static void
ia32_syscall(struct trapframe *framep)
{
	caddr_t params;
	int i;
	struct sysent *callp;
	struct thread *td = curthread;
	struct proc *p = td->td_proc;
	register_t orig_eflags;
	u_int sticks;
	int error;
	int narg;
	u_int32_t args[8];
	u_int64_t args64[8];
	u_int code;

	/*
	 * note: PCPU_LAZY_INC() can only be used if we can afford
	 * occassional inaccuracy in the count.
	 */
	cnt.v_syscall++;

	sticks = td->td_sticks;
	td->td_frame = framep;
	if (td->td_ucred != p->p_ucred) 
		cred_update_thread(td);
	params = (caddr_t)(framep->tf_special.sp & ((1L<<32)-1))
	    + sizeof(u_int32_t);
	code = framep->tf_scratch.gr8;		/* eax */
	orig_eflags = ia64_get_eflag();

	if (p->p_sysent->sv_prepsyscall) {
		/*
		 * The prep code is MP aware.
		 */
		(*p->p_sysent->sv_prepsyscall)(framep, args, &code, &params);
	} else {
		/*
		 * Need to check if this is a 32 bit or 64 bit syscall.
		 * fuword is MP aware.
		 */
		if (code == SYS_syscall) {
			/*
			 * Code is first argument, followed by actual args.
			 */
			code = fuword32(params);
			params += sizeof(int);
		} else if (code == SYS___syscall) {
			/*
			 * Like syscall, but code is a quad, so as to maintain
			 * quad alignment for the rest of the arguments.
			 * We use a 32-bit fetch in case params is not
			 * aligned.
			 */
			code = fuword32(params);
			params += sizeof(quad_t);
		}
	}

 	if (p->p_sysent->sv_mask)
 		code &= p->p_sysent->sv_mask;

 	if (code >= p->p_sysent->sv_size)
 		callp = &p->p_sysent->sv_table[0];
  	else
 		callp = &p->p_sysent->sv_table[code];

	narg = callp->sy_narg & SYF_ARGMASK;

	/*
	 * copyin and the ktrsyscall()/ktrsysret() code is MP-aware
	 */
	if (params != NULL && narg != 0)
		error = copyin(params, (caddr_t)args,
		    (u_int)(narg * sizeof(int)));
	else
		error = 0;

	for (i = 0; i < narg; i++)
		args64[i] = args[i];

#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSCALL))
		ktrsyscall(code, narg, args64);
#endif
	/*
	 * Try to run the syscall without Giant if the syscall
	 * is MP safe.
	 */
	if ((callp->sy_narg & SYF_MPSAFE) == 0)
		mtx_lock(&Giant);

	if (error == 0) {
		td->td_retval[0] = 0;
		td->td_retval[1] = framep->tf_scratch.gr10;	/* edx */

		STOPEVENT(p, S_SCE, narg);

		error = (*callp->sy_call)(td, args64);
	}

	switch (error) {
	case 0:
		framep->tf_scratch.gr8 = td->td_retval[0];	/* eax */
		framep->tf_scratch.gr10 = td->td_retval[1];	/* edx */
		ia64_set_eflag(ia64_get_eflag() & ~PSL_C);
		break;

	case ERESTART:
		/*
		 * Reconstruct pc, assuming lcall $X,y is 7 bytes,
		 * int 0x80 is 2 bytes. XXX Assume int 0x80.
		 */
		framep->tf_special.iip -= 2;
		break;

	case EJUSTRETURN:
		break;

	default:
 		if (p->p_sysent->sv_errsize) {
 			if (error >= p->p_sysent->sv_errsize)
  				error = -1;	/* XXX */
   			else
  				error = p->p_sysent->sv_errtbl[error];
		}
		framep->tf_scratch.gr8 = error;
		ia64_set_eflag(ia64_get_eflag() | PSL_C);
		break;
	}

	/*
	 * Traced syscall.
	 */
	if ((orig_eflags & PSL_T) && !(orig_eflags & PSL_VM)) {
		ia64_set_eflag(ia64_get_eflag() & ~PSL_T);
		trapsignal(td, SIGTRAP, 0);
	}

	/*
	 * Release Giant if we previously set it.
	 */
	if ((callp->sy_narg & SYF_MPSAFE) == 0)
		mtx_unlock(&Giant);

	/*
	 * Handle reschedule and other end-of-syscall issues
	 */
	userret(td, framep, sticks);

#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSRET))
		ktrsysret(code, error, td->td_retval[0]);
#endif

	/*
	 * This works because errno is findable through the
	 * register set.  If we ever support an emulation where this
	 * is not the case, this code will need to be revisited.
	 */
	STOPEVENT(p, S_SCX, code);

#ifdef DIAGNOSTIC
	cred_free_thread(td);
#endif
	WITNESS_WARN(WARN_PANIC, NULL, "System call %s returning",
	    (code >= 0 && code < SYS_MAXSYSCALL) ? syscallnames[code] : "???");
	mtx_assert(&sched_lock, MA_NOTOWNED);
	mtx_assert(&Giant, MA_NOTOWNED);
}
