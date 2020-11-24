# Walthrough of Kernel Initialization

## Overview

1. Calculating the Kernel Base
2. Linker Script
3. _locore.s_: Clearing _bss_
4. _locore.s_: Recovering the _bootinfo_ Structure
5. _locore.s_: Identifying the CPU Type
6. _locore.s_: Initializing Paging

## Calculating the Kernel Base and Load Address

Both the kernel base and load address are set within _locore.s_.

```c
/*
 * Compiled KERNBASE location and the kernel load address
 */
	.globl	kernbase
	.set	kernbase,KERNBASE
	.globl	kernload
	.set	kernload,KERNLOAD
```

### Kernel Base Address

```c
/* Located in /i386/include/vmparam.h */

#define	KERNBASE		VADDR(KPTDI, 0)

/* Located in /i386/include/pmap.h */

/*
 * Pte related macros
 */
#define VADDR(pdi, pti) ((vm_offset_t)(((pdi)<<PDRSHIFT)|((pti)<<PAGE_SHIFT)))

#ifndef NKPT
#ifdef PAE
#define	NKPT		120	/* actual number of kernel page tables */
#else
#define	NKPT		30	/* actual number of kernel page tables */
#endif
#endif
#ifndef NKPDE
#ifdef SMP
#define NKPDE	(KVA_PAGES - 1) /* number of page tables/pde's */
#else
#define NKPDE	(KVA_PAGES)	/* number of page tables/pde's */
#endif
#endif

/*
 * The *PTDI values control the layout of virtual memory
 *
 * XXX This works for now, but I am not real happy with it, I'll fix it
 * right after I fix locore.s and the magic 28K hole
 *
 * SMP_PRIVPAGES: The per-cpu address space is 0xff80000 -> 0xffbfffff
 */
#ifdef SMP
#define MPPTDI		(NPDEPTD-1)	/* per cpu ptd entry */
#define	KPTDI		(MPPTDI-NKPDE)	/* start of kernel virtual pde's */
#else
#define	KPTDI		(NPDEPTD-NKPDE)/* start of kernel virtual pde's */
#endif	/* SMP */
#define	PTDPTDI		(KPTDI-NPGPTD)	/* ptd entry that points to ptd! */

/* Located in /i386/include/param.h  */

#define PAGE_SHIFT	12		/* LOG2(PAGE_SIZE) */
#define PAGE_SIZE	(1<<PAGE_SHIFT)	/* bytes/page */
#define PAGE_MASK	(PAGE_SIZE-1)
#define NPTEPG		(PAGE_SIZE/(sizeof (pt_entry_t)))

#ifdef PAE
#define NPGPTD		4
#define PDRSHIFT	21		/* LOG2(NBPDR) */
#else
#define NPGPTD		1
#define PDRSHIFT	22		/* LOG2(NBPDR) */
#endif

#define NBPTD		(NPGPTD<<PAGE_SHIFT)
#define NPDEPTD		(NBPTD/(sizeof (pd_entry_t)))
```

Putting all this information together we obtain the following values:

```txt
VADDR(KPTDI, 0)

VADDR((NPDEPTD-NKPDE), 0)

VADDR(((NBPTD/(sizeof(pd_entry_t)))-NKPDE), 0)

We can easily reduce the value of sizeof(pd_entry_t) to 32. That leaves NKPDE,
whose value depends on whether or not PAE is enabled.

For PAE:

VADDR(((NBPTD/32)-512, 0)

VADDR(((4<<12)/32)-512, 0)

VADDR(((512-512), 0) = VADDR(0, 0) = 0x00000000

For !PAE:

VADDR(((1<<12)/32)-256, 0)

VADDR((128-256), 0) = VADDR(-128, 0)

Hence kernbase's virtual address is = -128<<21 = 0xf0000000.
```

### Kernel Load Address

The kernel's physical load address is much easier to calculate.

```c
/* Located in /i386/include/vmparam.h  */

/*
 * Kernel physical load address.
 */
#ifndef KERNLOAD
#define KERNLOAD	(2 << PDRSHIFT)
#endif

/* Located in /i386/include/param.h */

#ifdef PAE
#define PDRSHIFT	21	/* LOG2(NBPDR) */
#else
#define PDRSHIFT	22	/* LOG2(NBPDR) */
#endif
```
Hence, for PAE the physical load address is 0x400000 and for !PAE it is 0x800000.

## Linker Script

```c
/* $FreeBSD$ */
OUTPUT_FORMAT("elf32-i386-freebsd", "elf32-i386-freebsd", "elf32-i386-freebsd")
OUTPUT_ARCH(i386)
ENTRY(btext)
SEARCH_DIR(/usr/lib);
SECTIONS
{
  /* Read-only sections, merged into text segment:
  **  If PAE:
  **   kernbase = 0x00000000
  **   kernload = 0x00400000
  **  if !PAE:
  **   kernbase = 0xf0000000
  **   kernload = 0x00800000 
  */
  . = kernbase + kernload + SIZEOF_HEADERS;		/* PAE: 0x00400000, !PAE: 0xf0800000 */
  .interp     : { *(.interp) 	}
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .gnu.version   : { *(.gnu.version)	}
  .gnu.version_d   : { *(.gnu.version_d)	}
  .gnu.version_r   : { *(.gnu.version_r)	}
  .rel.text      :
    { *(.rel.text) *(.rel.gnu.linkonce.t*) }
  .rela.text     :
    { *(.rela.text) *(.rela.gnu.linkonce.t*) }
  .rel.data      :
    { *(.rel.data) *(.rel.gnu.linkonce.d*) }
  .rela.data     :
    { *(.rela.data) *(.rela.gnu.linkonce.d*) }
  .rel.rodata    :
    { *(.rel.rodata) *(.rel.gnu.linkonce.r*) }
  .rela.rodata   :
    { *(.rela.rodata) *(.rela.gnu.linkonce.r*) }
  .rel.got       : { *(.rel.got)		}
  .rela.got      : { *(.rela.got)		}
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.init      : { *(.rel.init)	}
  .rela.init     : { *(.rela.init)	}
  .rel.fini      : { *(.rel.fini)	}
  .rela.fini     : { *(.rela.fini)	}
  .rel.bss       : { *(.rel.bss)		}
  .rela.bss      : { *(.rela.bss)		}
  .rel.plt       : { *(.rel.plt)		}
  .rela.plt      : { *(.rela.plt)		}
  .init          : { *(.init)	} =0x9090
  .plt      : { *(.plt)	}
  .text      :
  {
    *(.text)
    *(.stub)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } =0x9090
  _etext = .;
  PROVIDE (etext = .);
  .fini      : { *(.fini)    } =0x9090
  .rodata    : { *(.rodata) *(.gnu.linkonce.r*) }
  .rodata1   : { *(.rodata1) }
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN(0x1000) + (. & (0x1000 - 1)) ; 
  .data    :
  {
    *(.data)
    *(.gnu.linkonce.d*)
    CONSTRUCTORS
  }
  .data1   : { *(.data1) }
  . = ALIGN(32 / 8);
  _start_ctors = .;
  PROVIDE (start_ctors = .);
  .ctors         :
  {
    *(.ctors)
  }
  _stop_ctors = .;
  PROVIDE (stop_ctors = .);
  .dtors         :
  {
    *(.dtors)
  }
  .got           : { *(.got.plt) *(.got) }
  .dynamic       : { *(.dynamic) }
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata     : { *(.sdata) }
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  .sbss      : { *(.sbss) *(.scommon) }
  .bss       :
  {
   *(.dynbss)
   *(.bss)
   *(COMMON)
  }
  . = ALIGN(32 / 8);
  _end = . ;
  PROVIDE (end = .);
  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* These must appear regardless of  .  */
}
```

## _locore.s_: Clearing _bss_

```c
/**********************************************************************
 *
 * This is where the bootblocks start us, set the ball rolling...
 *
 */
NON_GPROF_ENTRY(btext)

/* Tell the bios to warmboot next time */
	movw	$0x1234,0x472

/* Set up a real frame in case the double return in newboot is executed. */
	pushl	%ebp
	movl	%esp, %ebp

/* Don't trust what the BIOS gives for eflags. */
	pushl	$PSL_KERNEL		/* 0x00000002 */
	popfl

/*
 * Don't trust what the BIOS gives for %fs and %gs.  Trust the bootstrap
 * to set %cs, %ds, %es and %ss.
 */
	mov	%ds, %ax
	mov	%ax, %fs	/* %fs = %ds */
	mov	%ax, %gs	/* %fs = %ds */

/*
 * Clear the bss.  Not all boot programs do it, and it is our job anyway.
 *
 * XXX we don't check that there is memory for our bss and page tables
 * before using it.
 *
 * Note: we must be careful to not overwrite an active gdt or idt.  They
 * inactive from now until we switch to new ones, since we don't load any
 * more segment registers or permit interrupts until after the switch.
 *
 * Note: $R(foo) = ((foo)-KERNBASE)
 */
	movl	$R(end),%ecx	/* end = end of initialized data */
	movl	$R(edata),%edi	/* edata = end of uninitialized data */
	subl	%edi,%ecx		/* Calculate length of bss
	xorl	%eax,%eax
	cld						/* increment on string functions */
	rep
	stosb
```

## _locore.s_: Recovering the _bootinfo_ Structure

```c
/**********************************************************************
 *
 * Recover the bootinfo passed to us from the boot program
 *
 */
recover_bootinfo:
	/*
	 * This code is called in different ways depending on what loaded
	 * and started the kernel.  This is used to detect how we get the
	 * arguments from the other code and what we do with them.
	 *
	 * Old disk boot blocks:
	 *	(*btext)(howto, bootdev, cyloffset, esym);
	 *	[return address == 0, and can NOT be returned to]
	 *	[cyloffset was not supported by the FreeBSD boot code
	 *	 and always passed in as 0]
	 *	[esym is also known as total in the boot code, and
	 *	 was never properly supported by the FreeBSD boot code]
	 *
	 * Old diskless netboot code:
	 *	(*btext)(0,0,0,0,&nfsdiskless,0,0,0);
	 *	[return address != 0, and can NOT be returned to]
	 *	If we are being booted by this code it will NOT work,
	 *	so we are just going to halt if we find this case.
	 *
	 * New uniform boot code:
	 *	(*btext)(howto, bootdev, 0, 0, 0, &bootinfo)
	 *	[return address != 0, and can be returned to]
	 *
	 * There may seem to be a lot of wasted arguments in here, but
	 * that is so the newer boot code can still load very old kernels
	 * and old boot code can load new kernels.
	 */

	/*
	 * The old style disk boot blocks fake a frame on the stack and
	 * did an lret to get here.  The frame on the stack has a return
	 * address of 0.
	 */
	cmpl	$0,4(%ebp)
	je	olddiskboot		/* We don't jump here */

	/*
	 * We have some form of return address, so this is either the
	 * old diskless netboot code, or the new uniform code.  That can
	 * be detected by looking at the 5th argument, if it is 0
	 * we are being booted by the new uniform boot code.
	 */
	cmpl	$0,24(%ebp)
	je	newboot		/* Jump here */

	/*
	 * Seems we have been loaded by the old diskless boot code, we
	 * don't stand a chance of running as the diskless structure
	 * changed considerably between the two, so just halt.
	 */
	 hlt

	/*
	 * We have been loaded by the new uniform boot code.
	 * Let's check the bootinfo version, and if we do not understand
	 * it we return to the loader with a status of 1 to indicate this error
	 */
newboot:
	movl	28(%ebp),%ebx			/* &bootinfo */
	movl	BI_VERSION(%ebx),%eax	/* &bootinfo.version */
	cmpl	$1,%eax					/* We only understand version 1 */
	je	1f
	movl	$1,%eax					/* Return status */
	leave
	/*
	 * XXX this returns to our caller's caller (as is required) since
	 * we didn't set up a frame and our caller did.
	 */
	ret

1:
	/*
	 * If we have a kernelname copy it in
	 */
	movl	BI_KERNELNAME(%ebx),%esi	/* &bootinfo.kernelname */
	cmpl	$0,%esi
	je	2f								/* No kernelname */

	movl	$MAXPATHLEN,%ecx			/* Brute force!!! The string is
										** contained in bootinfo, not its
										** pointer. */
	movl	$R(kernelname),%edi
	cmpb	$'/',(%esi)					/* Make sure it starts with a slash */
	je	1f
	movb	$'/',(%edi)
	incl	%edi
	decl	%ecx
1:
	cld
	rep
	movsb		/* Copy the kernel name from bootinfo to kernelname */

2:
	/*
	 * Determine the size of the boot loader's copy of the bootinfo
	 * struct.  This is impossible to do properly because old versions
	 * of the struct don't contain a size field and there are 2 old
	 * versions with the same version number.
	 */
	movl	$BI_ENDCOMMON,%ecx		/* prepare for sizeless version */
	testl	$RB_BOOTINFO,8(%ebp)	/* bi_size (and bootinfo) valid? */
	je	got_bi_size		/* no, sizeless version */
	movl	BI_SIZE(%ebx),%ecx
got_bi_size:

	/*
	 * Copy the common part of the bootinfo struct
	 */
	movl	%ebx,%esi
	movl	$R(bootinfo),%edi
	cmpl	$BOOTINFO_SIZE,%ecx
	jbe	got_common_bi_size
	movl	$BOOTINFO_SIZE,%ecx
got_common_bi_size:
	cld
	rep
	movsb

#ifdef NFS_ROOT
#ifndef BOOTP_NFSV3
	/*
	 * If we have a nfs_diskless structure copy it in
	 */
	movl	BI_NFS_DISKLESS(%ebx),%esi
	cmpl	$0,%esi
	je	olddiskboot
	movl	$R(nfs_diskless),%edi
	movl	$NFSDISKLESS_SIZE,%ecx
	cld
	rep
	movsb
	movl	$R(nfs_diskless_valid),%edi
	movl	$1,(%edi)
#endif
#endif

	/*
	 * The old style disk boot.
	 *	(*btext)(howto, bootdev, cyloffset, esym);
	 * Note that the newer boot code just falls into here to pick
	 * up howto and bootdev, cyloffset and esym are no longer used
	 */
olddiskboot:
	movl	8(%ebp),%eax
	movl	%eax,R(boothowto)
	movl	12(%ebp),%eax
	movl	%eax,R(bootdev)

	ret
```

## _locore.s_: Identifying the CPU Type

```c
```

## _locore.s_: Initializing Paging


