/*	$NetBSD: mdreloc.c,v 1.5 2001/04/25 12:24:51 kleink Exp $	*/

/*-
 * Copyright (c) 2000 Eduardo Horvath.
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/mman.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "rtld.h"

/*
 * The following table holds for each relocation type:
 *	- the width in bits of the memory location the relocation
 *	  applies to (not currently used)
 *	- the number of bits the relocation value must be shifted to the
 *	  right (i.e. discard least significant bits) to fit into
 *	  the appropriate field in the instruction word.
 *	- flags indicating whether
 *		* the relocation involves a symbol
 *		* the relocation is relative to the current position
 *		* the relocation is for a GOT entry
 *		* the relocation is relative to the load address
 *
 */
#define _RF_S		0x80000000		/* Resolve symbol */
#define _RF_A		0x40000000		/* Use addend */
#define _RF_P		0x20000000		/* Location relative */
#define _RF_G		0x10000000		/* GOT offset */
#define _RF_B		0x08000000		/* Load address relative */
#define _RF_U		0x04000000		/* Unaligned */
#define _RF_SZ(s)	(((s) & 0xff) << 8)	/* memory target size */
#define _RF_RS(s)	( (s) & 0xff)		/* right shift */
static int reloc_target_flags[] = {
	0,							/* NONE */
	_RF_S|_RF_A|		_RF_SZ(8)  | _RF_RS(0),		/* RELOC_8 */
	_RF_S|_RF_A|		_RF_SZ(16) | _RF_RS(0),		/* RELOC_16 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* RELOC_32 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(8)  | _RF_RS(0),		/* DISP_8 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(16) | _RF_RS(0),		/* DISP_16 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* DISP_32 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP_30 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP_22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HI22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 13 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LO10 */
	_RF_G|			_RF_SZ(32) | _RF_RS(0),		/* GOT10 */
	_RF_G|			_RF_SZ(32) | _RF_RS(0),		/* GOT13 */
	_RF_G|			_RF_SZ(32) | _RF_RS(10),	/* GOT22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PC10 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PC22 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WPLT30 */
				_RF_SZ(32) | _RF_RS(0),		/* COPY */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* GLOB_DAT */
				_RF_SZ(32) | _RF_RS(0),		/* JMP_SLOT */
	      _RF_A|	_RF_B|	_RF_SZ(64) | _RF_RS(0),		/* RELATIVE */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(32) | _RF_RS(0),		/* UA_32 */

	      _RF_A|		_RF_SZ(32) | _RF_RS(0),		/* PLT32 */
	      _RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HIPLT22 */
	      _RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LOPLT10 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PCPLT32 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PCPLT22 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PCPLT10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 11 */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* 64 */
	_RF_S|_RF_A|/*extra*/	_RF_SZ(32) | _RF_RS(0),		/* OLO10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(42),	/* HH22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(32),	/* HM10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* LM22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(42),	/* PC_HH22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(32),	/* PC_HM10 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PC_LM22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP16 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP19 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* GLOB_JMP */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 7 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 5 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 6 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(64) | _RF_RS(0),		/* DISP64 */
	      _RF_A|		_RF_SZ(64) | _RF_RS(0),		/* PLT64 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HIX22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LOX10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(22),	/* H44 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(12),	/* M44 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* L44 */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* REGISTER */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(64) | _RF_RS(0),		/* UA64 */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(16) | _RF_RS(0),		/* UA16 */
};

#if 0
static const char *reloc_names[] = {
	"NONE", "RELOC_8", "RELOC_16", "RELOC_32", "DISP_8",
	"DISP_16", "DISP_32", "WDISP_30", "WDISP_22", "HI22",
	"22", "13", "LO10", "GOT10", "GOT13",
	"GOT22", "PC10", "PC22", "WPLT30", "COPY",
	"GLOB_DAT", "JMP_SLOT", "RELATIVE", "UA_32", "PLT32",
	"HIPLT22", "LOPLT10", "LOPLT10", "PCPLT22", "PCPLT32",
	"10", "11", "64", "OLO10", "HH22",
	"HM10", "LM22", "PC_HH22", "PC_HM10", "PC_LM22",
	"WDISP16", "WDISP19", "GLOB_JMP", "7", "5", "6",
	"DISP64", "PLT64", "HIX22", "LOX10", "H44", "M44",
	"L44", "REGISTER", "UA64", "UA16"
};
#endif

#define RELOC_RESOLVE_SYMBOL(t)		((reloc_target_flags[t] & _RF_S) != 0)
#define RELOC_PC_RELATIVE(t)		((reloc_target_flags[t] & _RF_P) != 0)
#define RELOC_BASE_RELATIVE(t)		((reloc_target_flags[t] & _RF_B) != 0)
#define RELOC_UNALIGNED(t)		((reloc_target_flags[t] & _RF_U) != 0)
#define RELOC_USE_ADDEND(t)		((reloc_target_flags[t] & _RF_A) != 0)
#define RELOC_TARGET_SIZE(t)		((reloc_target_flags[t] >> 8) & 0xff)
#define RELOC_VALUE_RIGHTSHIFT(t)	(reloc_target_flags[t] & 0xff)

static long reloc_target_bitmask[] = {
#define _BM(x)	(~(-(1ULL << (x))))
	0,				/* NONE */
	_BM(8), _BM(16), _BM(32),	/* RELOC_8, _16, _32 */
	_BM(8), _BM(16), _BM(32),	/* DISP8, DISP16, DISP32 */
	_BM(30), _BM(22),		/* WDISP30, WDISP22 */
	_BM(22), _BM(22),		/* HI22, _22 */
	_BM(13), _BM(10),		/* RELOC_13, _LO10 */
	_BM(10), _BM(13), _BM(22),	/* GOT10, GOT13, GOT22 */
	_BM(10), _BM(22),		/* _PC10, _PC22 */
	_BM(30), 0,			/* _WPLT30, _COPY */
	_BM(32), _BM(32), _BM(32),	/* _GLOB_DAT, JMP_SLOT, _RELATIVE */
	_BM(32), _BM(32),		/* _UA32, PLT32 */
	_BM(22), _BM(10),		/* _HIPLT22, LOPLT10 */
	_BM(32), _BM(22), _BM(10),	/* _PCPLT32, _PCPLT22, _PCPLT10 */
	_BM(10), _BM(11), -1,		/* _10, _11, _64 */
	_BM(10), _BM(22),		/* _OLO10, _HH22 */
	_BM(10), _BM(22),		/* _HM10, _LM22 */
	_BM(22), _BM(10), _BM(22),	/* _PC_HH22, _PC_HM10, _PC_LM22 */
	_BM(16), _BM(19),		/* _WDISP16, _WDISP19 */
	-1,				/* GLOB_JMP */
	_BM(7), _BM(5), _BM(6),		/* _7, _5, _6 */
	-1, -1,				/* DISP64, PLT64 */
	_BM(22), _BM(13),		/* HIX22, LOX10 */
	_BM(22), _BM(10), _BM(13),	/* H44, M44, L44 */
	-1, -1, _BM(16),		/* REGISTER, UA64, UA16 */
#undef _BM
};
#define RELOC_VALUE_BITMASK(t)	(reloc_target_bitmask[t])

#undef flush
#define	flush(va, offs)	\
	__asm __volatile("flush %0 + %1" : : "r" (va), "I" (offs));

static int reloc_nonplt_object(Obj_Entry *obj, const Elf_Rela *rela,
			       SymCache *cache);
static void install_plt(Elf_Half *pltgot, Elf_Addr proc);

extern char _rtld_bind_start_0[];
extern char _rtld_bind_start_1[];

int
do_copy_relocations(Obj_Entry *dstobj)
{
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	const Elf_Sym *dstsym;
	const Elf_Sym *srcsym;
	void *dstaddr;
	const void *srcaddr;
	Obj_Entry *srcobj;
	unsigned long hash;
	const char *name;
	size_t size;

	assert(dstobj->mainprog);   /* COPY relocations are invalid elsewhere */

	relalim = (const Elf_Rela *)((caddr_t)dstobj->rela + dstobj->relasize);
	for (rela = dstobj->rela; rela < relalim; rela++) {
		if (ELF_R_TYPE(rela->r_info) == R_SPARC_COPY) {
			dstaddr = (void *)(dstobj->relocbase + rela->r_offset);
			dstsym = dstobj->symtab + ELF_R_SYM(rela->r_info);
			name = dstobj->strtab + dstsym->st_name;
			hash = elf_hash(name);
			size = dstsym->st_size;

			for (srcobj = dstobj->next; srcobj != NULL;
			    srcobj = srcobj->next)
				if ((srcsym = symlook_obj(name, hash, srcobj,
				    false)) != NULL)
					break;

			if (srcobj == NULL) {
				_rtld_error("Undefined symbol \"%s\""
					    "referenced from COPY relocation"
					    "in %s", name, dstobj->path);
				return (-1);
			}

			srcaddr = (const void *)(srcobj->relocbase +
			    srcsym->st_value);
			memcpy(dstaddr, srcaddr, size);
		}
	}

	return (0);
}

int
reloc_non_plt(Obj_Entry *obj, Obj_Entry *obj_rtld)
{
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	SymCache *cache;
	int bytes = obj->nchains * sizeof(SymCache);
	int r = -1;

	/*
	 * The dynamic loader may be called from a thread, we have
	 * limited amounts of stack available so we cannot use alloca().
	 */
	cache = mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);
	if (cache == MAP_FAILED)
		cache = NULL;

	relalim = (const Elf_Rela *)((caddr_t)obj->rela + obj->relasize);
	for (rela = obj->rela; rela < relalim; rela++) {
		if (reloc_nonplt_object(obj, rela, cache) < 0)
			goto done;
	}
	r = 0;
done:
	if (cache)
		munmap(cache, bytes);
	return (r);
}

static int
reloc_nonplt_object(Obj_Entry *obj, const Elf_Rela *rela, SymCache *cache)
{
	const Obj_Entry *defobj;
	const Elf_Sym *def;
	Elf_Addr *where;
	Elf_Half *where32;
	Elf_Word type;
	Elf_Addr value;
	Elf_Addr mask;

	where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	where32 = (Elf_Half *)where;
	defobj = NULL;
	def = NULL;

	type = ELF_R_TYPE(rela->r_info);
	if (type == R_SPARC_NONE)
		return (0);

	/* We do JMP_SLOTs below */
	if (type == R_SPARC_JMP_SLOT)
		return (0);

	/* COPY relocs are also handled elsewhere */
	if (type == R_SPARC_COPY)
		return (0);

	/*
	 * Note: R_SPARC_UA16 must be numerically largest relocation type.
	 */
	if (type > R_SPARC_UA16)
		return (-1);

	value = rela->r_addend;

	/*
	 * Handle relative relocs here, because we might not
	 * be able to access globals yet.
	 */
	if (type == R_SPARC_RELATIVE) {
		/* XXXX -- apparently we ignore the preexisting value */
		*where = (Elf_Addr)(obj->relocbase + value);
		return (0);
	}

	/*
	 * If we get here while relocating rtld itself, we will crash because
	 * a non-local variable is accessed.
	 */
	if (RELOC_RESOLVE_SYMBOL(type)) {

		/* Find the symbol */
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    false, cache);
		if (def == NULL)
			return (-1);

		/* Add in the symbol's absolute address */
		value += (Elf_Addr)(defobj->relocbase + def->st_value);
	}

	if (RELOC_PC_RELATIVE(type))
		value -= (Elf_Addr)where;

	if (RELOC_BASE_RELATIVE(type)) {
		/*
		 * Note that even though sparcs use `Elf_rela' exclusively
		 * we still need the implicit memory addend in relocations
		 * referring to GOT entries. Undoubtedly, someone f*cked
		 * this up in the distant past, and now we're stuck with
		 * it in the name of compatibility for all eternity..
		 *
		 * In any case, the implicit and explicit should be mutually
		 * exclusive. We provide a check for that here.
		 */
		/* XXXX -- apparently we ignore the preexisting value */
		value += (Elf_Addr)(obj->relocbase);
	}

	mask = RELOC_VALUE_BITMASK(type);
	value >>= RELOC_VALUE_RIGHTSHIFT(type);
	value &= mask;

	if (RELOC_UNALIGNED(type)) {
		/* Handle unaligned relocations. */
		Elf_Addr tmp;
		char *ptr;
		int size;
		int i;

		size = RELOC_TARGET_SIZE(type) / 8;
		ptr = (char *)where;
		tmp = 0;

		/* Read it in one byte at a time. */
		for (i = 0; i < size; i++)
			tmp = (tmp << 8) | ptr[i];

		tmp &= ~mask;
		tmp |= value;

		/* Write it back out. */
		for (i = 0; i < size; i++)
			ptr[i] = ((tmp >> ((size - i - 1) * 8)) & 0xff);
	} else if (RELOC_TARGET_SIZE(type) > 32) {
		*where &= ~mask;
		*where |= value;
	} else {
		*where32 &= ~mask;
		*where32 |= value;
	}

	return (0);
}

int
reloc_plt(Obj_Entry *obj)
{
#if 0
	const Obj_Entry *defobj;
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	const Elf_Sym *def;
	Elf_Addr *where;
	Elf_Addr value;

	relalim = (const Elf_Rela *)((char *)obj->pltrela + obj->pltrelasize);
	for (rela = obj->pltrela; rela < relalim; rela++) {
		if (rela->r_addend == 0)
			continue;
		assert(ELF_R_TYPE(rela->r_info) == R_SPARC_JMP_SLOT);
		where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    true, NULL);
		value = (Elf_Addr)(defobj->relocbase + def->st_value);
		*where = value;
	}
#endif
	return (0);
}

/*
 * Instruction templates:
 */
#define	BAA	0x10400000	/*	ba,a	%xcc, 0 */
#define	SETHI	0x03000000	/*	sethi	%hi(0), %g1 */
#define	JMP	0x81c06000	/*	jmpl	%g1+%lo(0), %g0 */
#define	NOP	0x01000000	/*	sethi	%hi(0), %g0 */
#define	OR	0x82806000	/*	or	%g1, 0, %g1 */
#define	XOR	0x82c06000	/*	xor	%g1, 0, %g1 */
#define	MOV71	0x8283a000	/*	or	%o7, 0, %g1 */
#define	MOV17	0x9c806000	/*	or	%g1, 0, %o7 */
#define	CALL	0x40000000	/*	call	0 */
#define	SLLX	0x8b407000	/*	sllx	%g1, 0, %g1 */
#define	SETHIG5	0x0b000000	/*	sethi	%hi(0), %g5 */
#define	ORG5	0x82804005	/*	or	%g1, %g5, %g1 */


/* %hi(v) with variable shift */
#define	HIVAL(v, s)	(((v) >> (s)) &  0x003fffff)
#define LOVAL(v)	((v) & 0x000003ff)

int
reloc_jmpslots(Obj_Entry *obj)
{
	const Obj_Entry *defobj;
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	const Elf_Sym *def;
	Elf_Addr *where;
	Elf_Addr target;

	relalim = (const Elf_Rela *)((char *)obj->pltrela + obj->pltrelasize);
	for (rela = obj->pltrela; rela < relalim; rela++) {
		assert(ELF_R_TYPE(rela->r_info) == R_SPARC_JMP_SLOT);
		where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    true, NULL);
		if (def == NULL)
			return -1;
		target = (Elf_Addr)(defobj->relocbase + def->st_value);
		reloc_jmpslot(where, target, defobj, obj, (Elf_Rel *)rela);
	}
	obj->jmpslots_done = true;
	return (0);
}

Elf_Addr
reloc_jmpslot(Elf_Addr *wherep, Elf_Addr target, const Obj_Entry *obj,
	      const Obj_Entry *refobj, const Elf_Rel *rel)
{
	const Elf_Rela *rela = (const Elf_Rela *)rel;
	Elf_Addr offset;
	Elf_Half *where;

	if (rela - refobj->pltrela < 32764) {
		/*
		 * At the PLT entry pointed at by `where', we now construct
		 * a direct transfer to the now fully resolved function
		 * address.
		 *
		 * A PLT entry is supposed to start by looking like this:
		 *
		 *	sethi	(. - .PLT0), %g1
		 *	ba,a	%xcc, .PLT1
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *	nop
		 *
		 * When we replace these entries we start from the second
		 * entry and do it in reverse order so the last thing we
		 * do is replace the branch.  That allows us to change this
		 * atomically.
		 *
		 * We now need to find out how far we need to jump.  We
		 * have a choice of several different relocation techniques
		 * which are increasingly expensive.
		 */
		where = (Elf_Half *)wherep;
		offset = ((Elf_Addr)where) - target;
		if (offset <= (1L<<20) && offset >= -(1L<<20)) {
			/*
			 * We're within 1MB -- we can use a direct branch insn.
			 *
			 * We can generate this pattern:
			 *
			 *	sethi	%hi(. - .PLT0), %g1
			 *	ba,a	%xcc, addr
			 *	nop
			 *	nop
			 *	nop
			 *	nop
			 *	nop
			 *	nop
			 *
			 */
			where[1] = BAA | ((offset >> 2) &0x3fffff);
			flush(where, 4);
		} else if (target >= 0 && target < (1L<<32)) {
			/*
			 * We're withing 32-bits of address zero.
			 *
			 * The resulting code in the jump slot is:
			 *
			 *	sethi	%hi(. - .PLT0), %g1
			 *	sethi	%hi(addr), %g1
			 *	jmp	%g1+%lo(addr)
			 *	nop
			 *	nop
			 *	nop
			 *	nop
			 *	nop
			 *
			 */
			where[2] = JMP   | LOVAL(target);
			flush(where, 8);
			where[1] = SETHI | HIVAL(target, 10);
			flush(where, 4);
		} else if (target <= 0 && target > -(1L<<32)) {
			/*
			 * We're withing 32-bits of address -1.
			 *
			 * The resulting code in the jump slot is:
			 *
			 *	sethi	%hi(. - .PLT0), %g1
			 *	sethi	%hix(addr), %g1
			 *	xor	%g1, %lox(addr), %g1
			 *	jmp	%g1
			 *	nop
			 *	nop
			 *	nop
			 *	nop
			 *
			 */
			where[3] = JMP;
			flush(where, 12);
			where[2] = XOR | ((~target) & 0x00001fff);
			flush(where, 8);
			where[1] = SETHI | HIVAL(~target, 10);
			flush(where, 4);
		} else if (offset <= (1L<<32) && offset >= -((1L<<32) - 4)) {
			/*
			 * We're withing 32-bits -- we can use a direct call
			 * insn
			 *
			 * The resulting code in the jump slot is:
			 *
			 *	sethi	%hi(. - .PLT0), %g1
			 *	mov	%o7, %g1
			 *	call	(.+offset)
			 *	 mov	%g1, %o7
			 *	nop
			 *	nop
			 *	nop
			 *	nop
			 *
			 */
			where[3] = MOV17;
			flush(where, 12);
			where[2] = CALL	  | ((offset >> 4) & 0x3fffffff);
			flush(where, 8);
			where[1] = MOV71;
			flush(where, 4);
		} else if (offset >= 0 && offset < (1L<<44)) {
			/*
			 * We're withing 44 bits.  We can generate this pattern:
			 *
			 * The resulting code in the jump slot is:
			 *
			 *	sethi	%hi(. - .PLT0), %g1
			 *	sethi	%h44(addr), %g1
			 *	or	%g1, %m44(addr), %g1
			 *	sllx	%g1, 12, %g1
			 *	jmp	%g1+%l44(addr)
			 *	nop
			 *	nop
			 *	nop
			 *
			 */
			where[4] = JMP   | LOVAL(offset);
			flush(where, 16);
			where[3] = SLLX  | 12;
			flush(where, 12);
			where[2] = OR    | (((offset) >> 12) & 0x00001fff);
			flush(where, 8);
			where[1] = SETHI | HIVAL(offset, 22);
			flush(where, 4);
		} else if (offset < 0 && offset > -(1L<<44)) {
			/*
			 * We're withing 44 bits.  We can generate this pattern:
			 *
			 * The resulting code in the jump slot is:
			 *
			 *	sethi	%hi(. - .PLT0), %g1
			 *	sethi	%h44(-addr), %g1
			 *	xor	%g1, %m44(-addr), %g1
			 *	sllx	%g1, 12, %g1
			 *	jmp	%g1+%l44(addr)
			 *	nop
			 *	nop
			 *	nop
			 *
			 */
			where[4] = JMP   | LOVAL(offset);
			flush(where, 16);
			where[3] = SLLX  | 12;
			flush(where, 12);
			where[2] = XOR   | (((~offset) >> 12) & 0x00001fff);
			flush(where, 8);
			where[1] = SETHI | HIVAL(~offset, 22);
			flush(where, 4);
		} else {
			/*
			 * We need to load all 64-bits
			 *
			 * The resulting code in the jump slot is:
			 *
			 *	sethi	%hi(. - .PLT0), %g1
			 *	sethi	%hh(addr), %g1
			 *	sethi	%lm(addr), %g5
			 *	or	%g1, %hm(addr), %g1
			 *	sllx	%g1, 32, %g1
			 *	or	%g1, %g5, %g1
			 *	jmp	%g1+%lo(addr)
			 *	nop
			 *
			 */
			where[6] = JMP     | LOVAL(target);
			flush(where, 24);
			where[5] = ORG5;
			flush(where, 20);
			where[4] = SLLX    | 12;
			flush(where, 16);
			where[3] = OR      | LOVAL((target) >> 32);
			flush(where, 12);
			where[2] = SETHIG5 | HIVAL(target, 10);
			flush(where, 8);
			where[1] = SETHI   | HIVAL(target, 42);
			flush(where, 4);
		}
	} else {
		/*
		 * This is a high PLT slot; the relocation offset specifies a
		 * pointer that needs to be frobbed; no actual code needs to
		 * be modified. The pointer to be calculated needs the addend
		 * added and the reference object relocation base subtraced.
		 */
		*wherep = target + rela->r_addend -
		    (Elf_Addr)refobj->relocbase;
	}

	return (target);
}

/*
 * Install rtld function call into this PLT slot.
 */
#define	SAVE		0x9de3bf50
#define	SETHI_l0	0x21000000
#define	SETHI_l1	0x23000000
#define	OR_l0_l0	0xa0142000
#define	SLLX_l0_32_l0	0xa12c3020
#define	OR_l0_l1_l0	0xa0140011
#define	JMPL_l0_o1	0x93c42000
#define	MOV_g1_o0	0x90100001

void
init_pltgot(Obj_Entry *obj)
{
	Elf_Half *entry;

	if (obj->pltgot != NULL) {
		entry = (Elf_Half *)obj->pltgot;
		install_plt(&entry[0], (Elf_Addr)_rtld_bind_start_0);
		install_plt(&entry[8], (Elf_Addr)_rtld_bind_start_1);
		obj->pltgot[8] = (Elf_Addr)obj;
	}
}

static void
install_plt(Elf_Half *pltgot, Elf_Addr proc)
{
	pltgot[0] = SAVE;
	flush(pltgot, 0);
	pltgot[1] = SETHI_l0 | HIVAL(proc, 42);
	flush(pltgot, 4);
	pltgot[2] = SETHI_l1 | HIVAL(proc, 10);
	flush(pltgot, 8);
	pltgot[3] = OR_l0_l0 | LOVAL((proc) >> 32);
	flush(pltgot, 12);
	pltgot[4] = SLLX_l0_32_l0;
	flush(pltgot, 16);
	pltgot[5] = OR_l0_l1_l0;
	flush(pltgot, 20);
	pltgot[6] = JMPL_l0_o1 | LOVAL(proc);
	flush(pltgot, 24);
	pltgot[7] = MOV_g1_o0;
	flush(pltgot, 28);
}
