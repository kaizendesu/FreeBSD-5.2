/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *	from: hp300: @(#)pmap.h 7.2 (Berkeley) 12/16/90
 *	from: @(#)pmap.h        7.4 (Berkeley) 5/12/91
 *	from: FreeBSD: src/sys/i386/include/pmap.h,v 1.70 2000/11/30
 * $FreeBSD$
 */

#ifndef	_MACHINE_PMAP_H_
#define	_MACHINE_PMAP_H_

#include <sys/queue.h>
#include <machine/cache.h>
#include <machine/tte.h>

#define	PMAP_CONTEXT_MAX	8192

#define	pmap_page_is_mapped(m)	(!TAILQ_EMPTY(&(m)->md.tte_list))

typedef	struct pmap *pmap_t;

struct md_page {
	TAILQ_HEAD(, tte) tte_list;
	struct	pmap *pmap;
	uint32_t colors[DCACHE_COLORS];
	int32_t	color;
	uint32_t flags;
};

struct pmap {
	struct	tte *pm_tsb;
	vm_object_t pm_tsb_obj;
	u_int	pm_active;
	u_int	pm_context[MAXCPU];
	struct	pmap_statistics pm_stats;
};

void	pmap_bootstrap(vm_offset_t ekva);
vm_paddr_t pmap_kextract(vm_offset_t va);
void	pmap_kenter(vm_offset_t va, vm_page_t m);
void	pmap_kremove(vm_offset_t);
void	pmap_kenter_flags(vm_offset_t va, vm_paddr_t pa, u_long flags);
void	pmap_kremove_flags(vm_offset_t va);

int	pmap_cache_enter(vm_page_t m, vm_offset_t va);
void	pmap_cache_remove(vm_page_t m, vm_offset_t va);

int	pmap_remove_tte(struct pmap *pm1, struct pmap *pm2, struct tte *tp,
			vm_offset_t va);
int	pmap_protect_tte(struct pmap *pm1, struct pmap *pm2, struct tte *tp,
			 vm_offset_t va);

void	pmap_map_tsb(void);

void	pmap_clear_write(vm_page_t m);

#define	vtophys(va)	pmap_kextract(((vm_offset_t)(va)))

extern	vm_paddr_t avail_start;
extern	vm_paddr_t avail_end;
extern	struct pmap kernel_pmap_store;
#define	kernel_pmap	(&kernel_pmap_store)
extern	vm_paddr_t phys_avail[];
extern	vm_offset_t virtual_avail;
extern	vm_offset_t virtual_end;

extern	vm_paddr_t msgbuf_phys;

static __inline int
pmap_track_modified(pmap_t pm, vm_offset_t va)
{
	if (pm == kernel_pmap)
		return ((va < kmi.clean_sva) || (va >= kmi.clean_eva));
	else
		return (1);
}

#ifdef PMAP_STATS

SYSCTL_DECL(_debug_pmap_stats);

#define	PMAP_STATS_VAR(name) \
	static long name; \
	SYSCTL_LONG(_debug_pmap_stats, OID_AUTO, name, CTLFLAG_RW, \
	    &name, 0, "")

#define	PMAP_STATS_INC(var) \
	atomic_add_long(&var, 1)

#else

#define	PMAP_STATS_VAR(name)
#define	PMAP_STATS_INC(var)

#endif

#endif /* !_MACHINE_PMAP_H_ */
