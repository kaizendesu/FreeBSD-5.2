/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * Copyright (c) 1989, 1990 William Jolitz
 * Copyright (c) 1994 John Dyson
 * Copyright (c) 2001 Jake Burkholder.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)vm_machdep.c	7.3 (Berkeley) 5/13/91
 *	Utah $Hdr: vm_machdep.c 1.16.1.1 89/06/23$
 * 	from: FreeBSD: src/sys/i386/i386/vm_machdep.c,v 1.167 2001/07/12
 * $FreeBSD$
 */

#include "opt_kstack_pages.h"
#include "opt_pmap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/linker_set.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/sf_buf.h>
#include <sys/sysctl.h>
#include <sys/unistd.h>
#include <sys/user.h>
#include <sys/vmmeter.h>

#include <dev/ofw/openfirm.h>

#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_param.h>
#include <vm/uma.h>
#include <vm/uma_int.h>

#include <machine/cache.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/fp.h>
#include <machine/fsr.h>
#include <machine/frame.h>
#include <machine/md_var.h>
#include <machine/ofw_machdep.h>
#include <machine/ofw_mem.h>
#include <machine/tlb.h>
#include <machine/tstate.h>

static void	sf_buf_init(void *arg);
SYSINIT(sock_sf, SI_SUB_MBUF, SI_ORDER_ANY, sf_buf_init, NULL)

/*
 * Expanded sf_freelist head. Really an SLIST_HEAD() in disguise, with the
 * sf_freelist head with the sf_lock mutex.
 */
static struct {
	SLIST_HEAD(, sf_buf) sf_head;
	struct mtx sf_lock;
} sf_freelist;

static u_int	sf_buf_alloc_want;

PMAP_STATS_VAR(uma_nsmall_alloc);
PMAP_STATS_VAR(uma_nsmall_alloc_oc);
PMAP_STATS_VAR(uma_nsmall_free);

void
cpu_exit(struct thread *td)
{
	struct md_utrap *ut;
	struct proc *p;

	p = td->td_proc;
	p->p_md.md_sigtramp = NULL;
	if ((ut = p->p_md.md_utrap) != NULL) {
		ut->ut_refcnt--;
		if (ut->ut_refcnt == 0)
			free(ut, M_SUBPROC);
		p->p_md.md_utrap = NULL;
	}
}

void
cpu_sched_exit(struct thread *td)
{
	struct vmspace *vm;
	struct pcpu *pc;
	struct proc *p;

	mtx_assert(&sched_lock, MA_OWNED);

	p = td->td_proc;
	vm = p->p_vmspace;
	if (vm->vm_refcnt > 1)
		return;
	SLIST_FOREACH(pc, &cpuhead, pc_allcpu) {
		if (pc->pc_vmspace == vm) {
			vm->vm_pmap.pm_active &= ~pc->pc_cpumask;
			vm->vm_pmap.pm_context[pc->pc_cpuid] = -1;
			pc->pc_vmspace = NULL;
		}
	}
}

void
cpu_thread_exit(struct thread *td)
{
}

void
cpu_thread_clean(struct thread *td)
{
}

void
cpu_thread_setup(struct thread *td)
{
	struct pcb *pcb;

	pcb = (struct pcb *)((td->td_kstack + KSTACK_PAGES * PAGE_SIZE -
	    sizeof(struct pcb)) & ~0x3fUL);
	td->td_frame = (struct trapframe *)pcb - 1;
	td->td_pcb = pcb;
}

void
cpu_thread_swapin(struct thread *td)
{
}

void
cpu_thread_swapout(struct thread *td)
{
}

void
cpu_set_upcall(struct thread *td, struct thread *td0)
{
	struct trapframe *tf;
	struct frame *fr;
	struct pcb *pcb;

	bcopy(td0->td_frame, td->td_frame, sizeof(struct trapframe));

	pcb = td->td_pcb;
	tf = td->td_frame;
	fr = (struct frame *)tf - 1;
	fr->fr_local[0] = (u_long)fork_return;
	fr->fr_local[1] = (u_long)td;
	fr->fr_local[2] = (u_long)tf;
	pcb->pcb_pc = (u_long)fork_trampoline - 8;
	pcb->pcb_sp = (u_long)fr - SPOFF;
}

void
cpu_set_upcall_kse(struct thread *td, struct kse_upcall *ku)
{
	struct trapframe *tf;
	uint64_t sp;

	tf = td->td_frame;
	sp = (uint64_t)ku->ku_stack.ss_sp + ku->ku_stack.ss_size;
	tf->tf_out[0] = (uint64_t)ku->ku_mailbox;
	tf->tf_out[6] = sp - SPOFF - sizeof(struct frame);
	tf->tf_tpc = (uint64_t)ku->ku_func;
	tf->tf_tnpc = tf->tf_tpc + 4;

	td->td_retval[0] = tf->tf_out[0];
	td->td_retval[1] = tf->tf_out[1];
}

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb, set up the stack so that the child
 * ready to run and return to user mode.
 */
void
cpu_fork(struct thread *td1, struct proc *p2, struct thread *td2, int flags)
{
	struct md_utrap *ut;
	struct trapframe *tf;
	struct frame *fp;
	struct pcb *pcb1;
	struct pcb *pcb2;
	vm_offset_t sp;
	int error;
	int i;

	KASSERT(td1 == curthread || td1 == &thread0,
	    ("cpu_fork: p1 not curproc and not proc0"));

	if ((flags & RFPROC) == 0)
		return;

	p2->p_md.md_sigtramp = td1->td_proc->p_md.md_sigtramp;
	if ((ut = td1->td_proc->p_md.md_utrap) != NULL)
		ut->ut_refcnt++;
	p2->p_md.md_utrap = ut;

	/* The pcb must be aligned on a 64-byte boundary. */
	pcb1 = td1->td_pcb;
	pcb2 = (struct pcb *)((td2->td_kstack + KSTACK_PAGES * PAGE_SIZE -
	    sizeof(struct pcb)) & ~0x3fUL);
	td2->td_pcb = pcb2;

	/*
	 * Ensure that p1's pcb is up to date.
	 */
	critical_enter();
	if ((td1->td_frame->tf_fprs & FPRS_FEF) != 0)
		savefpctx(pcb1->pcb_ufp);
	critical_exit();
	/* Make sure the copied windows are spilled. */
	flushw();
	/* Copy the pcb (this will copy the windows saved in the pcb, too). */
	bcopy(pcb1, pcb2, sizeof(*pcb1));

	/*
	 * If we're creating a new user process and we're sharing the address
	 * space, the parent's top most frame must be saved in the pcb.  The
	 * child will pop the frame when it returns to user mode, and may
	 * overwrite it with its own data causing much suffering for the
	 * parent.  We check if its already in the pcb, and if not copy it
	 * in.  Its unlikely that the copyin will fail, but if so there's not
	 * much we can do.  The parent will likely crash soon anyway in that
	 * case.
	 */
	if ((flags & RFMEM) != 0 && td1 != &thread0) {
		sp = td1->td_frame->tf_sp;
		for (i = 0; i < pcb1->pcb_nsaved; i++) {
			if (pcb1->pcb_rwsp[i] == sp)
				break;
		}
		if (i == pcb1->pcb_nsaved) {
			error = copyin((caddr_t)sp + SPOFF, &pcb1->pcb_rw[i],
			    sizeof(struct rwindow));
			if (error == 0) {
				pcb1->pcb_rwsp[i] = sp;
				pcb1->pcb_nsaved++;
			}
		}
	}

	/*
	 * Create a new fresh stack for the new process.
	 * Copy the trap frame for the return to user mode as if from a
	 * syscall.  This copies most of the user mode register values.
	 */
	tf = (struct trapframe *)pcb2 - 1;
	bcopy(td1->td_frame, tf, sizeof(*tf));

	tf->tf_out[0] = 0;			/* Child returns zero */
	tf->tf_out[1] = 0;
	tf->tf_tstate &= ~TSTATE_XCC_C;		/* success */
	tf->tf_fprs = 0;

	td2->td_frame = tf;
	fp = (struct frame *)tf - 1;
	fp->fr_local[0] = (u_long)fork_return;
	fp->fr_local[1] = (u_long)td2;
	fp->fr_local[2] = (u_long)tf;
	pcb2->pcb_sp = (u_long)fp - SPOFF;
	pcb2->pcb_pc = (u_long)fork_trampoline - 8;

	/*
	 * Now, cpu_switch() can schedule the new process.
	 */
}

void
cpu_reset(void)
{
	static char bspec[64] = "";
	phandle_t chosen;
	static struct {
		cell_t	name;
		cell_t	nargs;
		cell_t	nreturns;
		cell_t	bootspec;
	} args = {
		(cell_t)"boot",
		1,
		0,
		(cell_t)bspec
	};
	if ((chosen = OF_finddevice("/chosen")) != 0) {
		if (OF_getprop(chosen, "bootpath", bspec, sizeof(bspec)) == -1)
			bspec[0] = '\0';
		bspec[sizeof(bspec) - 1] = '\0';
	}

	openfirmware_exit(&args);
}

/*
 * Intercept the return address from a freshly forked process that has NOT
 * been scheduled yet.
 *
 * This is needed to make kernel threads stay in kernel mode.
 */
void
cpu_set_fork_handler(struct thread *td, void (*func)(void *), void *arg)
{
	struct frame *fp;
	struct pcb *pcb;

	pcb = td->td_pcb;
	fp = (struct frame *)(pcb->pcb_sp + SPOFF);
	fp->fr_local[0] = (u_long)func;
	fp->fr_local[1] = (u_long)arg;
}

int
is_physical_memory(vm_paddr_t addr)
{
	struct ofw_mem_region *mr;

	for (mr = sparc64_memreg; mr < sparc64_memreg + sparc64_nmemreg; mr++)
		if (addr >= mr->mr_start && addr < mr->mr_start + mr->mr_size)
			return (1);
	return (0);
}

/*
 * Allocate a pool of sf_bufs (sendfile(2) or "super-fast" if you prefer. :-))
 */
static void
sf_buf_init(void *arg)
{
	struct sf_buf *sf_bufs;
	vm_offset_t sf_base;
	int i;

	mtx_init(&sf_freelist.sf_lock, "sf_bufs list lock", NULL, MTX_DEF);
	SLIST_INIT(&sf_freelist.sf_head);
	sf_base = kmem_alloc_nofault(kernel_map, nsfbufs * PAGE_SIZE);
	sf_bufs = malloc(nsfbufs * sizeof(struct sf_buf), M_TEMP,
	    M_NOWAIT | M_ZERO);
	for (i = 0; i < nsfbufs; i++) {
		sf_bufs[i].kva = sf_base + i * PAGE_SIZE;
		SLIST_INSERT_HEAD(&sf_freelist.sf_head, &sf_bufs[i], free_list);
	}
	sf_buf_alloc_want = 0;
}

/*
 * Get an sf_buf from the freelist. Will block if none are available.
 */
struct sf_buf *
sf_buf_alloc(struct vm_page *m)
{
	struct sf_buf *sf;
	int error;

	mtx_lock(&sf_freelist.sf_lock);
	while ((sf = SLIST_FIRST(&sf_freelist.sf_head)) == NULL) {
		sf_buf_alloc_want++;
		error = msleep(&sf_freelist, &sf_freelist.sf_lock, PVM|PCATCH,
		    "sfbufa", 0);
		sf_buf_alloc_want--;

		/*
		 * If we got a signal, don't risk going back to sleep. 
		 */
		if (error)
			break;
	}
	if (sf != NULL) {
		SLIST_REMOVE_HEAD(&sf_freelist.sf_head, free_list);
		sf->m = m;
		pmap_qenter(sf->kva, &sf->m, 1);
	}
	mtx_unlock(&sf_freelist.sf_lock);
	return (sf);
}

/*
 * Detatch mapped page and release resources back to the system.
 */
void
sf_buf_free(void *addr, void *args)
{
	struct sf_buf *sf;
	struct vm_page *m;

	sf = args;
	pmap_qremove((vm_offset_t)addr, 1);
	m = sf->m;
	vm_page_lock_queues();
	vm_page_unwire(m, 0);
	/*
	 * Check for the object going away on us. This can
	 * happen since we don't hold a reference to it.
	 * If so, we're responsible for freeing the page.
	 */
	if (m->wire_count == 0 && m->object == NULL)
		vm_page_free(m);
	vm_page_unlock_queues();
	sf->m = NULL;
	mtx_lock(&sf_freelist.sf_lock);
	SLIST_INSERT_HEAD(&sf_freelist.sf_head, sf, free_list);
	if (sf_buf_alloc_want > 0)
		wakeup_one(&sf_freelist);
	mtx_unlock(&sf_freelist.sf_lock);
}

void
swi_vm(void *v)
{

	/*
	 * Nothing to do here yet - busdma bounce buffers are not yet
	 * implemented.
	 */
}

void *
uma_small_alloc(uma_zone_t zone, int bytes, u_int8_t *flags, int wait)
{
	static vm_pindex_t color;
	vm_paddr_t pa;
	vm_page_t m;
	int pflags;
	void *va;

	PMAP_STATS_INC(uma_nsmall_alloc);

	*flags = UMA_SLAB_PRIV;

	if ((wait & (M_NOWAIT|M_USE_RESERVE)) == M_NOWAIT)
		pflags = VM_ALLOC_INTERRUPT;
	else
		pflags = VM_ALLOC_SYSTEM;

	if (wait & M_ZERO)
		pflags |= VM_ALLOC_ZERO;

	for (;;) {
		m = vm_page_alloc(NULL, color++, pflags | VM_ALLOC_NOOBJ);
		if (m == NULL) {
			if (wait & M_NOWAIT)
				return (NULL);
			else
				VM_WAIT;
		} else
			break;
	}

	pa = VM_PAGE_TO_PHYS(m);
	if (m->md.color != DCACHE_COLOR(pa)) {
		KASSERT(m->md.colors[0] == 0 && m->md.colors[1] == 0,
		    ("uma_small_alloc: free page still has mappings!"));
		PMAP_STATS_INC(uma_nsmall_alloc_oc);
		m->md.color = DCACHE_COLOR(pa);
		dcache_page_inval(pa);
	}
	va = (void *)TLB_PHYS_TO_DIRECT(pa);
	if ((wait & M_ZERO) && (m->flags & PG_ZERO) == 0)
		bzero(va, PAGE_SIZE);
	return (va);
}

void
uma_small_free(void *mem, int size, u_int8_t flags)
{
	vm_page_t m;

	PMAP_STATS_INC(uma_nsmall_free);
	m = PHYS_TO_VM_PAGE(TLB_DIRECT_TO_PHYS((vm_offset_t)mem));
	vm_page_lock_queues();
	vm_page_free(m);
	vm_page_unlock_queues();
}
