# Walthrough of *loader*

## Overview

1. Memory Map: Transferring from _boot2_
2. Linking _loader_
3. _kargs_ Structure
4. Obtain Length of Base and Extended Memory
5. Initializing the Console
6. Initializing the Block Cache
7. Initializing Devices
8. Detect ACPI
9. Setting the Current Device
10. Obtaining the BIOS Segment Map
11. Setting the Architecture Switch
12. Start _loader_ Interface 

## Memory Map: Transferring from _boot2_

```txt
___________________________________________________ 0x600
|                                                 |
|                     Boot0                       |
|_________________________________________________| 0x700
|                                                 |
|                Relocated Boot1                  |
|_________________________________________________| 0x900
|                                                 |
|                   Arguments                     |
|_________________________________________________|
|                                                 |
|               Supervisor Stack                  |
|_________________________________________________| 0x1800
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|-------------------------------------------------| 0x1e00
|                                                 |
|           Interrupt Descriptor Table            |
|_________________________________________________| 0x1fa0
|                                                 |
|            Task-State Segment (TSS)             |
|_________________________________________________| 0x2130
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|-------------------------------------------------| 0x4000
|                                                 |
|                 Page Directory                  |
|_________________________________________________| 0x5000
|                                                 |
|                   Page Tables                   |
|_________________________________________________|
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|-------------------------------------------------| 0x8cec
|                                                 |
|                     Boot1                       |
|_________________________________________________| 0x8eec
|                                                 |
|                   Disk Label                    |
|_________________________________________________| 0x9000
|                                                 |
|                   BTX Server                    |
|_________________________________________________|
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|-------------------------------------------------| 0xc000
|                                                 |
|                   BTX Client                    |
|_________________________________________________|
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|-------------------------------------------------|
|                                                 |
|                  User Stack                     |
|_________________________________________________| 0x9f000
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|-------------------------------------------------| 0x200000
|                                                 |
|                  Loader                         |
|_________________________________________________|
```

## Linking _loader_

### Makefile.inc Files

```make
# Common defines for all of /sys/boot/i386/
#
# $FreeBSD$

LOADER_ADDRESS?=	0x200000
CFLAGS+=		-ffreestanding -mpreferred-stack-boundary=2

.if ${MACHINE_ARCH} == "amd64"
.MAKEFLAGS:  MACHINE_ARCH=i386 MACHINE=i386 REALLY_AMD64=true
.endif

.if defined(REALLY_AMD64) && !defined(__been_to_Makefile_inc)
__been_to_Makefile_inc=	1
CFLAGS+=		-m32
LDFLAGS+=		-m elf_i386_fbsd
AFLAGS+=		--32
.endif

# $FreeBSD$

SRCS+=	bcache.c boot.c commands.c console.c devopen.c interp.c 
SRCS+=	interp_backslash.c interp_parse.c ls.c misc.c 
SRCS+=	module.c panic.c

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "amd64"
SRCS+=	load_elf32.c load_elf64.c
.endif
.if ${MACHINE_ARCH} == "powerpc"
SRCS+=	load_elf32.c
.endif
.if ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "ia64" || ${MACHINE_ARCH} == "alpha"
SRCS+=	load_elf64.c
.endif

.if defined(LOADER_NET_SUPPORT)
SRCS+=	dev_net.c
.endif

# Machine-independant ISA PnP
.if HAVE_ISABUS
SRCS+=	isapnp.c
.endif
.if HAVE_PNP
SRCS+=	pnp.c
.endif

# Forth interpreter
.if BOOT_FORTH
SRCS+=	interp_forth.c
MAN+=	../forth/loader.conf.5
MAN+=	../forth/loader.4th.8
.endif

MAN+=	loader.8
```

### _loader_ Makefile

```make
# $FreeBSD$

PROG=		loader
STRIP=
NEWVERSWHAT=	"bootstrap loader" i386
BINDIR?=	/boot
INSTALLFLAGS=	-b

# architecture-specific loader code
SRCS=		main.c conf.c

CFLAGS+=	-ffreestanding
# Enable Preboot Execution Environment (PXE) Trivial File Transfer Protocol (TFTP)
# or Network Filesystem (NFS) support, not both.
.if defined(LOADER_TFTP_SUPPORT)
CFLAGS+=	-DLOADER_TFTP_SUPPORT
.else
CFLAGS+=	-DLOADER_NFS_SUPPORT
.endif

# Enable Plug 'n Play (PnP) and ISA-PnP code.
HAVE_PNP=	yes
HAVE_ISABUS=	yes

.if !defined(NOFORTH)
# Enable BootForth
BOOT_FORTH=	yes
CFLAGS+=	-DBOOT_FORTH -I${.CURDIR}/../../ficl -I${.CURDIR}/../../ficl/i386
.if exists(${.OBJDIR}/../../ficl/libficl.a)
LIBFICL=	${.OBJDIR}/../../ficl/libficl.a
.else
LIBFICL=	${.CURDIR}/../../ficl/libficl.a
.endif
.endif

.if defined(LOADER_BZIP2_SUPPORT)
CFLAGS+=	-DLOADER_BZIP2_SUPPORT
.endif
.if !defined(LOADER_NO_GZIP_SUPPORT)
CFLAGS+=	-DLOADER_GZIP_SUPPORT
.endif

# Always add Machine-Independent (MI) sources 
.PATH:		${.CURDIR}/../../common
.include	<${.CURDIR}/../../common/Makefile.inc>
CFLAGS+=	-I${.CURDIR}/../../common
CFLAGS+=	-I${.CURDIR}/../../.. -I.

CLEANFILES+=	vers.c vers.o ${PROG}.list ${PROG}.bin ${PROG}.sym ${PROG}.help

CFLAGS+=	-Wall
LDFLAGS=	-nostdlib -static -Ttext 0x0

# i386 standalone support library
LIBI386=	${.OBJDIR}/../libi386/libi386.a
CFLAGS+=	-I${.CURDIR}/..

# where to get libstand from
#XXX need a better way to do this
LIBSTAND=	${.CURDIR}/../../../../lib/libstand/libstand.a
.if !exists(${LIBSTAND})
LIBSTAND=	${.OBJDIR}/../../../../lib/libstand/libstand.a
.if !exists(${LIBSTAND})
LIBSTAND=	-lstand
.endif
.endif
CFLAGS+=	-I${.CURDIR}/../../../../lib/libstand/

# BTX components
.if exists(${.OBJDIR}/../btx)
BTXDIR=		${.OBJDIR}/../btx
.else
BTXDIR=		${.CURDIR}/../btx
.endif
BTXLDR=		${BTXDIR}/btxldr/btxldr
BTXKERN=	${BTXDIR}/btx/btx
BTXCRT=		${BTXDIR}/lib/crt0.o
CFLAGS+=	-I${.CURDIR}/../btx/lib

# BTX is expecting ELF components
CFLAGS+=	-elf

# Debug me!
#CFLAGS+=	-g
#LDFLAGS+=	-g

vers.o:	${.CURDIR}/../../common/newvers.sh ${.CURDIR}/version
	sh ${.CURDIR}/../../common/newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${CC} ${CFLAGS} -c vers.c

${PROG}: ${PROG}.bin ${BTXLDR} ${BTXKERN} ${BTXCRT}
	btxld -v -f aout -e ${LOADER_ADDRESS} -o ${.TARGET} -l ${BTXLDR} \
		-b ${BTXKERN} ${PROG}.bin
#	/usr/bin/kzip ${.TARGET}
#	mv ${.TARGET}.kz ${.TARGET}

${PROG}.bin: ${PROG}.sym
	cp ${.ALLSRC} ${.TARGET}
	strip -R .comment -R .note ${.TARGET}

${PROG}.help: help.common help.i386
	cat ${.ALLSRC} | awk -f ${.CURDIR}/../../common/merge_help.awk > ${.TARGET}

.PATH: ${.CURDIR}/../../forth 
FILES=	${PROG}.help loader.4th support.4th loader.conf
FILES+= screen.4th frames.4th beastie.4th
FILESDIR_loader.conf=	/boot/defaults

.if !exists(${DESTDIR}/boot/loader.rc)
FILES+=	${.CURDIR}/loader.rc
.endif

.include <${.CURDIR}/../Makefile.inc>

# Cannot use ${OBJS} above this line
.include <bsd.prog.mk>

${PROG}.sym: ${OBJS} ${LIBI386} ${LIBSTAND} ${LIBFICL} vers.o
	${CC} ${LDFLAGS} -o ${.TARGET} ${BTXCRT} ${OBJS} vers.o \
		${LIBFICL} ${LIBI386} ${LIBSTAND}

# If it's not there, don't consider it a target
.if exists(${.CURDIR}/../../../i386/include)
beforedepend ${OBJS}: machine

machine:
	ln -sf ${.CURDIR}/../../../i386/include machine

.endif

CLEANFILES+=	machine
```

### _btxldr_ Makefile (_loader_ entry point)

```make
# $FreeBSD$

M4?=	m4
M4FLAGS+=	-DLOADER_ADDRESS=${LOADER_ADDRESS}

.if defined(BTXLDR_VERBOSE)
M4FLAGS+=	-DBTXLDR_VERBOSE
.endif

all: btxldr

btxldr: btxldr.o
	${LD} -N -e start -Ttext ${LOADER_ADDRESS} -o btxldr.out btxldr.o
	objcopy -S -O binary btxldr.out ${.TARGET}

btxldr.o: btxldr.s
	(cd ${.CURDIR}; ${M4} ${M4FLAGS} btxldr.s ) | \
		${AS} ${AFLAGS} -o ${.TARGET}

CLEANFILES+= btxldr btxldr.out btxldr.o

.include <bsd.prog.mk>
```

## Initializing BTX Environment with _btxldr_

### _btxldr_: Initial User Stack

```txt
          User Stack
_____________________________________ 0x9f000
|                                   |
| int autoboot                      |
|___________________________________|
|                                   |
| ino_t ino                         |
|___________________________________|
|                                   |
| return address to main()          |
|___________________________________|
|                                   |
| union hdr                         |
|___________________________________|
|                                   |
| Elf32_Phdr ep                     |
|___________________________________|
|                                   |
| Elf32_Shdr es                     |
|___________________________________|
|                                   |
| caddr_t p                         |
|___________________________________|
|                                   |
| uint32_t addr, x                  |
|___________________________________|
|                                   |
| int fmt, i, j                     |            Supervisor Stack
|___________________________________|  __________________________________________ 0x1800
|                                   |  |                                        |
| uint32 VTOP(&bootinfo)            |  | SS (SEL_UDATA)                         |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| uint32 0, 0, 0                    |  | User ESP Offset                        |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| uint32 MAKEBOOTDEV                |  | EFLAGS                                 |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| RB_BOOTINFO | (opts & RBX_MASK)   |  | CS (SEL_UCODE)                         |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| caddr_t addr (loader entry)       |  | EIP                                    |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| return address to load()          |  | Error Code                             |
|___________________________________|  |________________________________________|
```

MAKE A MAP OF \_\_EXEC'S ARGURMENTS STARTING FROM EBP

### _btxldr_: Loading a Provisional GDT

It is important to note that we set the top of the client stack to 0xa0000.

```c
start:
		cld
		movl $m_logo,%esi		# %esi = "\nBTX Loader 1.00 "
		call putstr
		movzwl BDA_MEM,%eax		# %eax = 0x413, BIOS ext. mem location
		shll $0xa,%eax			# Convert free ext. mem to bytes
		movl %eax,%ebp			# %ebp points to top of client stack
								# We assume that %ebp = 0xa0000
ifdef(`BTXLDR_VERBOSE',`
		movl $m_mem,%esi		# %esi = Starting in protected mode (base mem=\0)\n"
		call hexout
		call putstr				# Print available user memory
')
		lgdt gdtdesc			# Load new GDT

	.
	.
	.

#
# Global descriptor table.
#
gdt:	.word 0x0,0x0,0x0,0x0			# Null entry
		.word 0xffff,0x0,0x9a00,0xcf	# SEL_SCODE
		.word 0xffff,0x0,0x9200,0xcf	# SEL_SDATA
		.word 0xffff,0x0,0x9a00,0x0		# SEL_RCODE
		.word 0xffff,0x0,0x9200,0x0		# SEL_RDATA
gdt.1:
gdtdesc:	.word gdt.1-gdt-1		# Limit
		.long gdt			# Base
```

### _btxldr_: Relocating _bootinfo_ and _exec_ Arguments

```asm
ifdef(`BTXLDR_VERBOSE',`
		movl $m_esp,%esi		# %esi = "Arguments passed (esp=\0):\n"
		movl %esp,%eax			# %eax = caller's (boot2) stack pointer
		call hexout
		call putstr				# Print boot2's esp
		movl $m_args,%esi		# %esi = "<howto= bootdev= junk=   bootinfo=\0>\n"
		leal 0x4(%esp,1),%ebx		# %ebx = Address of 1st karg
		movl $0x6,%ecx				# Set count ot 6 args
start.1:
		movl (%ebx),%eax		# %eax = current arg
		addl $0x4,%ebx			# Increment arg pointer
		call hexout				# Display current arg
		loop start.1
		call putstr				# Print karg list
')
#
# Relocate bootinfo
#
		movl $0x48,%ecx 		# Set count to 56 bytes
		subl %ecx,%ebp			# Decrement callee stack (0xa0000 - 0x48)
		movl 0x18(%esp,1),%esi	# %esi = VTOP(&bootinfo)
		cmpl $0x0, %esi
		je start_null_bi		# Jump if ptr is NULL
		movl %ebp,%edi			# %edi = base of allocated stack memory
		rep
		movsb					# Relocate bootinfo args to the stack
								#  at address 0x9FFB8
		movl %ebp,0x18(%esp,1)	# Update &bootinfo
ifdef(`BTXLDR_VERBOSE',`
		movl $m_rel_bi,%esi		# Display
		movl %ebp,%eax			#  bootinfo
		call hexout			#  relocation
		call putstr			#  message
')
#
# Relocate exec arguments
#
start_null_bi:
		movl $0x18,%ecx 		# Set count to 24 bytes
		subl %ecx,%ebp			# Decrement client stack
		leal 0x4(%esp,1),%esi	# %esi = base of __exec arguments
		movl %ebp,%edi			# Base of allocated stack memory
		rep
		movsb					# Relocate __exec's arguments
								#  at address 0x9FFA0
ifdef(`BTXLDR_VERBOSE',`
		movl $m_rel_args,%esi		# Display
		movl %ebp,%eax			#  argument
		call hexout			#  relocation
		call putstr			#  message
')
```

### _btxldr_: Initializing BTX kernel

_btx's_ header is located at the end of _btxldr's_ data segment, which is
denoted as MEM\_DATA in the code. The _btx_hdr_ structure is defined as follows: 

```c
/*
 * BTX image header.
 */
struct btx_hdr {
    uint8_t	btx_machid;		/* Machine ID */
    uint8_t	btx_hdrsz;		/* Header size */
    uint8_t	btx_magic[3];		/* Magic */
    uint8_t	btx_majver;		/* Major version */
    uint8_t	btx_minver;		/* Minor version */
    uint8_t	btx_flags;		/* Flags */
    uint16_t	btx_pgctl;		/* Paging control */
    uint16_t	btx_textsz;		/* Text size */
    uint32_t	btx_entry;		/* Client entry address */
};
```

It is important to keep this structure in mind as we read through the
initialization code.

```asm
#
# Now that we've relocated the caller's arguments, we can change %esp
#
		movl $MEM_ESP,%esp		# %esp = 0x1000
		movl $MEM_DATA,%ebx		# %ebx = base of btx header
		movl $m_vers,%esi		# %esi = "BTX version is \0\n"
		call putstr
		movb 0x5(%ebx),%al		# Get major version
		addb $'0',%al
		call putchr				# Display major version
		movb $'.',%al
		call putchr				# Display '.'
		movb 0x6(%ebx),%al		# Get minor
		xorb %ah,%ah
		movb $0xa,%dl
		divb %dl,%al			# Divide minor version by 0xa
		addb $'0',%al
		call putchr
		movb %ah,%al
		addb $'0',%al
		call putchr				# Display (minor version / 10)
		call putstr				# Print message

		movl %ebx,%esi					# %esi = base of BTX kernel header
		movzwl 0x8(%ebx),%edi			# %edi = btx_pgctl 
		orl $PAG_SIZ/PAG_ENT-1,%edi		# %edi |= 0x3ff
										# The 10 least sig bits of %edi
		incl %edi						# Incr %edi to obtain the rounded page number
		shll $0x2,%edi					# Multiply page number by 4 to obtain
										# offset
		addl $MEM_TBL,%edi				# Add offset to base of page tables
										# at 0x5000
		pushl %edi						# Save load address
										# %edi = 0x9000
		movzwl 0xa(%ebx),%ecx			# %ecx = btx_textsz
ifdef(`BTXLDR_VERBOSE',`
		pushl %ecx			# Save image size
')
		rep
		movsb				# Relocate BTX kernel to 0x9000
		movl %esi,%ebx		# %esi = base of BTX kernel
ifdef(`BTXLDR_VERBOSE',`
		movl $m_rel_btx,%esi	# Restore
		popl %eax				#  parameters
		call hexout				#  and
')
		popl %ebp				# %ebp = 0x9000
ifdef(`BTXLDR_VERBOSE',`
		movl %ebp,%eax			#  the
		call hexout				#  relocation
		call putstr				#  message
')
```

### _btxldr_: Setting Up the Client

Like the previous section, it is vital to have the structure of the header in 
mind as we read through the code.

```c
/*
 * ELF header. Located /sys/sys/elf32.h
 */

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* File identification. */
	Elf32_Half	e_type;		/* File type. */
	Elf32_Half	e_machine;	/* Machine architecture. */
	Elf32_Word	e_version;	/* ELF format version. */
	Elf32_Addr	e_entry;	/* Entry point. */
	Elf32_Off	e_phoff;	/* Program header file offset. */
	Elf32_Off	e_shoff;	/* Section header file offset. */
	Elf32_Word	e_flags;	/* Architecture-specific flags. */
	Elf32_Half	e_ehsize;	/* Size of ELF header in bytes. */
	Elf32_Half	e_phentsize;	/* Size of program header entry. */
	Elf32_Half	e_phnum;	/* Number of program header entries. */
	Elf32_Half	e_shentsize;	/* Size of section header entry. */
	Elf32_Half	e_shnum;	/* Number of section header entries. */
	Elf32_Half	e_shstrndx;	/* Section name strings section. */
} Elf32_Ehdr;
```

```asm
		addl $PAG_SIZ,%ebp		# %ebp = base address of client
								#  Note: BTX kernel size = PAG_SIZ
ifdef(`BTXLDR_VERBOSE',`
		movl $m_base,%esi		#  %esi = "Client base address is \0\n"
		movl %ebp,%eax
		call hexout
		call putstr				# Print base address
')
#
# Set up ELF-format client program.
#
		cmpl $0x464c457f,(%ebx) 	# ELF magic number? {0x7f, 'E', 'L', 'F'}
		je start.3					# Jump if client is ELF file
		movl $e_fmt,%esi			# "Error: Client format not supported\n"
		call putstr
start.2:	jmp start.2				# Infinite loop
start.3:
ifdef(`BTXLDR_VERBOSE',`
		movl $m_elf,%esi		# "Client format is ELF\n"
		call putstr
		movl $m_segs,%esi		# "text segment: offset="
')
		movl $0x2,%edi			# Segment count
		movl 0x1c(%ebx),%edx	# Get program header table offset
		addl %ebx,%edx			# %edx = program header table
		movzwl 0x2c(%ebx),%ecx		# Get # of program header entries

start.4:
		cmpl $0x1,(%edx)		# Is p_type PT_LOAD?
		jne start.6				# No

ifdef(`BTXLDR_VERBOSE',`
		movl 0x4(%edx),%eax		# Display
		call hexout			#  p_offset
		movl 0x8(%edx),%eax		# Display
		call hexout			#  p_vaddr
		movl 0x10(%edx),%eax		# Display
		call hexout			#  p_filesz
		movl 0x14(%edx),%eax		# Display
		call hexout			#  p_memsz
		call putstr			# End message
')
		pushl %esi			# Save
		pushl %edi			#  working
		pushl %ecx			#  registers

		movl 0x4(%edx),%esi		# Get segment offset
		addl %ebx,%esi			# %esi = pointer to segment
		movl 0x8(%edx),%edi		# Get virtual address of segment
		addl %ebp,%edi			# %edi = pointer to segment (vaddr)
		movl 0x10(%edx),%ecx	# Get segment's size in bytes

		rep
		movsb					# Relocate segment to its virtual address
		movl 0x14(%edx),%ecx		# Any bytes of memory to add?
		subl 0x10(%edx),%ecx		# %ecx = p_memsz - p_filesz

		jz start.5			# Jump if %ecx == 0
		xorb %al,%al
		rep
		stosb				# Zero segment's memory
start.5:
		popl %ecx			# Restore
		popl %edi			#  working
		popl %esi			#  registers
		decl %edi			# Decrement segments to relocate
		je start.7			# Jump if done
start.6:
		addl $0x20,%edx 		# To next entry
		loop start.4			# Till done

start.7:
ifdef(`BTXLDR_VERBOSE',`
		movl $m_done,%esi		# Display done
		call putstr			#  message
')
		movl $start.8,%esi			# Real mode stub
		movl $MEM_STUB,%edi			# %edi = 0x600
		movl $start.9-start.8,%ecx	# Size of real mode stub
		rep
		movsb						# Relocate real mode stub
		ljmp $SEL_RCODE,$MEM_STUB	# Jump to 0x18:0x600
		.code16
start.8:
		xorw %ax,%ax			# Data
		movb $SEL_RDATA,%al		#  selector; %al = 0x20
		movw %ax,%ss			# Reload SS
		movw %ax,%ds			# Reset
		movw %ax,%es			#  other
		movw %ax,%fs			#  segment
		movw %ax,%gs			#  limits
		movl %cr0,%eax			# Switch to
		decw %ax				#  real
		movl %eax,%cr0			#  mode
		ljmp $0,$MEM_ENTRY		# Jump to [0x0:0x9010]
start.9:
		.code32
```

After jumping to the MEM\_ENTRY, we reinitialize the _btx_ kernel and use the
TSS to switch to the client code _loader_. Recall that before we use _iret_ to
switch, we calculate the base of the stack in %eax using the BIOS Data Area and
subtract this value by 0x1000 to create space for relocating _BOOTINFO_ and
_exec_ arguments.

```c
		movl $MEM_USR,%edx			# %edx = 0x0000a000
		movzwl %ss:BDA_MEM,%eax 	# Free KiBs starting at addr 0x0
									# We assume this returns 0x280
		shll $0xa,%eax				# Convert free memory count to bytes
									# %eax = 0xa0000
		subl $0x1000,%eax			# %eax = 0x9f000
									# We subtract 0x1000 from the top of avail
									# memory so that we can relocate the bootinfo
									# arguments without overwriting the boot2's
									# user stack
```

In order to pass the _BOOTINFO_ and _exec_ arguments to the new client _loader_,
a small section of code is executed before calling the _main_ function. This
code is from /sys/boot/i386/btx/lib/btxcsu.s and stands for btx client set up.

```asm
#
# BTX C startup code (ELF).
#

#
# Globals.
#
		.global _start
#
# Constants.
#
		.set ARGADJ,0xfa0		# Argument adjustment
#
# Client entry point.
#
_start: 
		movl %eax,__base		# Set base address
		movl %esp,%eax			# Set
		addl $ARGADJ,%eax		#  argument
		movl %eax,__args		#  pointer
		call main			# Invoke client main()
		call exit			# Invoke client exit()
```

This code sets the values for external variables *\_\_args* and *\_\_base*,
which denote the ESP value of the _bootinfo_/_exec_ arguments and the base of
the client stack respectively.

The reason we add ARGADJ to *\_\_base* is simple. The client stack begins 0x1000
below the the top of extended memory. _bootinfo_ and the _exec_ arguments are
just below the top of extended memory 0xa0000 at 0xa0000 - 0x60 = 0x9ffa0.

The client stack begins at 0xa0000 - 0x1000 = 0x9f000, hence we must add 0xfa0
to access the _bootinfo_ and _exec_ arguments. This allows us to pass the
boot arguments from _boot0_ all the way to _/boot/loader_. Now we can finally
examine the C code for *loader* in full detail.

## _kargs_ Structure

```c
/* Arguments passed in from the boot1/boot2 loader */
static struct 
{
    u_int32_t	howto;
    u_int32_t	bootdev;
    u_int32_t	bootflags;
    u_int32_t	pxeinfo;
    u_int32_t	res2;
    u_int32_t	bootinfo;
} *kargs;
```

## _main.c_: Supervisor and User Stack

```txt
          User Stack
_____________________________________ 0x9f000
|                                   |
| int autoboot                      |
|___________________________________|
|                                   |
| ino_t ino                         |
|___________________________________|
|                                   |
| return address to main()          |
|___________________________________|
|                                   |
| union hdr                         |
|___________________________________|
|                                   |
| Elf32_Phdr ep                     |
|___________________________________|
|                                   |
| Elf32_Shdr es                     |
|___________________________________|
|                                   |
| caddr_t p                         |
|___________________________________|
|                                   |
| uint32_t addr, x                  |
|___________________________________|
|                                   |
| int fmt, i, j                     |            Supervisor Stack
|___________________________________|  __________________________________________ 0x1800
|                                   |  |                                        |
| uint32 VTOP(&bootinfo)            |  | SS (SEL_UDATA)                         |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| uint32 0, 0, 0                    |  | User ESP Offset                        |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| uint32 MAKEBOOTDEV                |  | EFLAGS                                 |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| RB_BOOTINFO | (opts & RBX_MASK)   |  | CS (SEL_UCODE)                         |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| return address to intx30          |  | EIP                                    |
|___________________________________|  |________________________________________|
|                                   |  |                                        |
| int i                             |  | Error Code                             |
|___________________________________|  |________________________________________|
```

## _main.c_: Obtain Length of Base and Extended Memory

```c
struct smap {
	u_int64_t	base;
	u_int64_t	length;
	u_int64_t	type;
} __packed;

static struct smap smap;

void
bios_getmem(void)
{
    /* Parse system memory map */
    v86.ebx = 0;

	/* BIOS Call: INT 0x15, AX = 0xe820
	**  Uses ES:DI to point to Address Range Descriptor
	**  structure to fill in (see e820map in hdr.h)
	**
	**  Sets CF = 1 when there is an error, 0 otherwise
	**  Sets eax to `SMAP' for verification
	**  Sets ebx to 0 if it reached the last Address
	**  range descriptor
	*/
    do {
		v86.ctl = V86_FLAGS;
		v86.addr = 0x15;		/* int 0x15 function 0xe820*/
		v86.eax = 0xe820;
		v86.ecx = sizeof(struct smap);
		v86.edx = SMAPSIG;		/* SMAPSIG = 0x534d4150 */
		v86.es = VTOPSEG(&smap);
		v86.edi = VTOPOFF(&smap);
		v86int();
		if ((v86.efl & 1) || (v86.eax != SMAPSIG))	// Error or INT not supported
	    	break;
		/* look for a low-memory segment that's large enough (1000 sectors) */
		if ((smap.type == 1) && (smap.base == 0) && (smap.length >= (512 * 1024)))
	    	bios_basemem = smap.length;
		/* look for the first segment in 'extended' memory */
		if ((smap.type == 1) && (smap.base == 0x100000)) {
	    	bios_extmem = smap.length;
		}
    } while (v86.ebx != 0);

    /* Fall back to the old compatibility function for base memory
	** if e820 returns length 0 for it. INT 0x12 returns size of
	** contiguous memory starting at address 0x0 in KiBs.
	*/
    if (bios_basemem == 0) {
		v86.ctl = 0;
		v86.addr = 0x12;		/* int 0x12 */
		v86int();
	
		bios_basemem = (v86.eax & 0xffff) * 1024;
    }

    /* Fall back through several compatibility functions for extended memory.
	**
	** BIOS Call: INT 0x15, AX = 0xe801
	**  Phoenix BIOS 4.0- Get memory size for >64M configurations.
	**
	**  CF = 0 if successful
	**     = 1 if error
	**  AX = extended memory btw. 1M and 16M in Kibs
	**  BX = extended memory > 16M in 64 KiB blocks
	**  CX = configured memory 1M to 16M in Kibs
	**  DX = configured memory above 16M in 64 Kib blocks
	*/
    if (bios_extmem == 0) {
		v86.ctl = V86_FLAGS;
		v86.addr = 0x15;		/* int 0x15 function 0xe801*/
		v86.eax = 0xe801;
		v86int();
	if (!(v86.efl & 1)) {
	    bios_extmem = ((v86.ecx & 0xffff) + ((v86.edx & 0xffff) * 64)) * 1024;
	}
    }

	/* BIOS Call: INT 0x15, AH = 0x88
	**  Get extended memory size (286+)
	**
	**  CF = 0 if successful
	**     = 1 if error
	**  AX = number of contiguous KiB starting at 0x100000
	**  AH = status
	**       0x80 invalid command (PC,PCjr)
	**       0x86 unsupported funciton (XT,PS30)  
	*/
    if (bios_extmem == 0) {
		v86.ctl = 0;
		v86.addr = 0x15;		/* int 0x15 function 0x88*/
		v86.eax = 0x8800;
		v86int();
		bios_extmem = (v86.eax & 0xffff) * 1024;
    }

    /* Set memtop to actual top of memory */
    memtop = 0x100000 + bios_extmem;
}
```

## _main.c_: Initializing the Console

### _console_ Structure

```c
/*
 * Modular console support.
 */
struct console
{
	const char	*c_name;
	const char	*c_desc;
	int			c_flags;
#define C_PRESENTIN		(1<<0)
#define C_PRESENTOUT	(1<<1)
#define C_ACTIVEIN		(1<<2)
#define C_ACTIVEOUT		(1<<3)
	void		(* c_probe)(struct console *cp);	/* set c_flags to match hardware */
	int			(* c_init)(int arg);				/* reinit XXX may need more args */
	void		(* c_out)(int c);					/* emit c */
	int			(* c_in)(void);						/* wait for and return input */
	int			(* c_ready)(void);					/* return nonzer if input waiting */
};

extern struct console vidconsole;
extern struct console comconsole;
extern struct console nullconsole;

struct console *consoles[] = {
	&vidconsole, /* Defined in /libi386/vidconsole.c */
	&comconsole, /* Defined in /libi386/comconsole.c */
	&nullconsole, /* Defined in /libi386/nullconsole.c */
	NULL
};
```

### _main.c_: Console Initialization Code

```c
    /* 
     * XXX Chicken-and-egg problem; we want to have console output early, but some
     * console attributes may depend on reading from eg. the boot device, which we
     * can't do yet.
     *
     * We can use printf() etc. once this is done.
     * If the previous boot stage has requested a serial console, prefer that.
     */
    if (initial_howto & RB_SERIAL)
	setenv("console", "comconsole", 1);
    if (initial_howto & RB_MUTE)
	setenv("console", "nullconsole", 1);
    cons_probe();

	.
	.
	.
}

// Located in /boot/common/console.c

static int	cons_set(struct env_var *ev, int flags, void *value);
static int	cons_find(char *name);

/*
 * Detect possible console(s) to use.  The first probed console
 * is marked active.  Also create the console variable.
 *	
 * XXX Add logic for multiple console support.
 */
void
cons_probe(void) 
{
    int			cons;
    int			active;
    char		*prefconsole;
    
    /* Do all console probes */
    for (cons = 0; consoles[cons] != NULL; cons++) {
	consoles[cons]->c_flags = 0;
 	consoles[cons]->c_probe(consoles[cons]);	// Set flags to match hardware
    }
    /* Now find the first working one */
    active = -1;
    for (cons = 0; consoles[cons] != NULL && active == -1; cons++) {
	consoles[cons]->c_flags = 0;
 	consoles[cons]->c_probe(consoles[cons]);
	if (consoles[cons]->c_flags == (C_PRESENTIN | C_PRESENTOUT))	// c_flags == 0x3?
	    active = cons;
    }

    /* Check to see if a console preference has already been registered */
    prefconsole = getenv("console");
    if (prefconsole != NULL)
	prefconsole = strdup(prefconsole);
    if (prefconsole != NULL) {
	unsetenv("console");		/* we want to replace this */
	for (cons = 0; consoles[cons] != NULL; cons++)
	    /* look for the nominated console, use it if it's functional */
	    if (!strcmp(prefconsole, consoles[cons]->c_name) &&
		(consoles[cons]->c_flags == (C_PRESENTIN | C_PRESENTOUT)))
		active = cons;
	free(prefconsole);
    }
    if (active == -1)
		active = 0;
    consoles[active]->c_flags |= (C_ACTIVEIN | C_ACTIVEOUT);
    consoles[active]->c_init(0);

    printf("Console: %s\n", consoles[active]->c_desc);
    env_setenv("console", EV_VOLATILE, consoles[active]->c_name, cons_set,
	env_nounset);
}


/*
** This is what the static printf uses to write to a console, and thus is the
** dependency on an active console mentioned in the FreeBSD comment
*/
void
putchar(int c)
{
	int		cons;

	/* Expand newlines */
	if (c == '\n')
		putchar('\r');

	for (cons = 0; consoles[cons] != NULL; cons++)
		if (consoles[cons]->c_flags & C_ACTIVEOUT)
			consoles[cons]->c_out(c);
}
```

## _main.c_: Initializing the Block Cache

```c
struct bcachectl
{
    daddr_t	bc_blkno;
    time_t	bc_stamp;
    int		bc_count;
};

static struct bcachectl	*bcache_ctl;
static caddr_t		bcache_data;
static bitstr_t		*bcache_miss;
static u_int		bcache_nblks;
static u_int		bcache_blksize;
static u_int		bcache_hits, bcache_misses, bcache_ops, bcache_bypasses;
static u_int		bcache_flushes;
static u_int		bcache_bcount;

static void	bcache_invalidate(daddr_t blkno);
static void	bcache_insert(caddr_t buf, daddr_t blkno);
static int	bcache_lookup(caddr_t buf, daddr_t blkno);

/*
 * Initialise the cache for (nblks) of (bsize).
 */
int
bcache_init(u_int nblks, size_t bsize)
{
    /* discard any old contents */
    if (bcache_data != NULL) {
	free(bcache_data);
	bcache_data = NULL;
	free(bcache_ctl);
    }

    /* Allocate control structures */
    bcache_nblks = nblks;
    bcache_blksize = bsize;
    bcache_data = malloc(bcache_nblks * bcache_blksize);	// Allocate blocks
    bcache_ctl = (struct bcachectl *)malloc(bcache_nblks * sizeof(struct bcachectl));	// Allocate block headers
    bcache_miss = bit_alloc((bcache_nblks + 1) / 2);

	// If malloc failed to allocate any of these structures...
    if ((bcache_data == NULL) || (bcache_ctl == NULL) || (bcache_miss == NULL)) {
	if (bcache_miss)
	    free(bcache_miss);
	if (bcache_ctl)
	    free(bcache_ctl);
	if (bcache_data)
	    free(bcache_data);
	bcache_data = NULL;
	return(ENOMEM);
    }

    return(0);
}
```

## _main.c_: Initializing Devices

### _devsw_ Table

Each _devsw_ entry in this table has its own definition, so there isn't a single
definition for a _devsw_ structure.

```c
// Located in /boot/i386/loader/conf.c

/* Exported for libstand */
struct devsw *devsw[] = {
	&bioscd,
	&biosdisk,
#if defined(LOADER_NFS_SUPPORT) || defined(LOADER_TFTP_SUPPORT)
	&pxedisk,
#endif
	NULL
};
```
### Probing the _devsw_ Table

```c
    /*
     * March through the device switch probing for things. For each
	 * non-null entry if the initialization functione exists, call it.
     */
    for (i = 0; devsw[i] != NULL; i++)
		if (devsw[i]->dv_init != NULL)
	    	(devsw[i]->dv_init)();
    printf("BIOS %dkB/%dkB available memory\n", bios_basemem / 1024, bios_extmem / 1024);
    if (initial_bootinfo != NULL) {
		initial_bootinfo->bi_basemem = bios_basemem / 1024;
		initial_bootinfo->bi_extmem = bios_extmem / 1024;
    }
```

## _main.c_: Detect ACPI

### _RSDP_\_\_DESCRIPTOR_ Structure

```c
// Located in /contrib/dev/acpica/actbl.h

/*
 * Architecture-independent tables
 * The architecture dependent tables are in separate files
 */
typedef struct rsdp_descriptor /* Root System Descriptor Pointer */
{
	char	Signature [8];	/* ACPI signature, contains "RSD PTR " */
	UINT8	Checksum;				/* To make sum of struct == 0 */
	char	OemId [6];				/* OEM identification */
	UINT8	Revision				/* Must be 0 for 1.0, 2 for 2.0 */
	UINT32	RsdtPhysicalAddress		/* 32-bit physical address of RSDT */
	UINT32	Length;					/* XSDT Length in bytes including hdr */
	UINT64	XsdtPhysicalAddress;	/* 64-bit physical address of XSDT */
	UINT8	ExtendedChecksum;		/* Checksum of entire table */
	char	Reserved [3];			/* Reserved field must be 0 */
} RSDP_DESCRIPTOR;
```

### Code

```c
void
biosacpi_detect(void)
{
    RSDP_DESCRIPTOR	*rsdp;
    char		buf[16];
    int			revision;

    /* XXX check the BIOS datestamp */

    /* locate and validate the RSDP */
    if ((rsdp = biosacpi_find_rsdp()) == NULL)
		return;

    /* export values from the RSDP */
    revision = rsdp->Revision;
    if (revision == 0)
		revision = 1;
    sprintf(buf, "%d", revision);
    setenv("hint.acpi.0.revision", buf, 1);
    sprintf(buf, "%6s", rsdp->OemId);
    buf[6] = '\0';
    setenv("hint.acpi.0.oem", buf, 1);
    sprintf(buf, "0x%08x", rsdp->RsdtPhysicalAddress);
    setenv("hint.acpi.0.rsdt", buf, 1);
    if (revision >= 2) {
		/* XXX extended checksum? */
		sprintf(buf, "0x%016llx", rsdp->XsdtPhysicalAddress);
		setenv("hint.acpi.0.xsdt", buf, 1);
		sprintf(buf, "%d", rsdp->Length);
		setenv("hint.acpi.0.xsdt_length", buf, 1);
    }
    /* XXX other tables? */

    setenv("acpi_load", "YES", 1);
}

/*
 * Find the RSDP in low memory.
 */
static RSDP_DESCRIPTOR *
biosacpi_find_rsdp(void)
{
    RSDP_DESCRIPTOR	*rsdp;

    /* search the Extended BIOS Data Area */
    if ((rsdp = biosacpi_search_rsdp((char *)0, 0x400)) != NULL)
	return(rsdp);

    /* search the BIOS space */
    if ((rsdp = biosacpi_search_rsdp((char *)0xe0000, 0x20000)) != NULL)
	return(rsdp);

    return(NULL);
}

static rsdp_descriptor *
biosacpi_search_rsdp(char *base, int length)
{
    rsdp_descriptor	*rsdp;
    u_int8_t		*cp, sum;
    int			ofs, idx;

    /* search on 16-byte boundaries */
    for (ofs = 0; ofs < length; ofs += 16) {
	rsdp = (rsdp_descriptor *)(base + ofs);

	/* compare signature, validate checksum
	** rsdp_sig = "RSDP Ptr"
	*/
	if (!strncmp(rsdp->signature, rsdp_sig, strlen(rsdp_sig))) {
	    cp = (u_int8_t *)rsdp;
	    sum = 0;
	    for (idx = 0; idx < rsdp_checksum_length; idx++)
			sum += *(cp + idx);
	    if (sum != 0) {
			printf("acpi: bad rsdp checksum (%d)\n", sum);
			continue;
	    }
	    return(rsdp);
	}
    }
    return(null);
}
``` 

## _main.c_: Setting the Current Device

### _i386_devdesc_ Structure

```c
/*
 * i386 fully-qualified device descriptor.
 * Note, this must match the 'struct devdesc' declaration
 * in bootstrap.h.
 */
struct i386_devdesc
{
    struct devsw	*d_dev;
    int				d_type;
    union 
    {
		struct 
		{
	    	int		unit;
	    	int		slice;
	    	int		partition;
	    	void	*data;
		} biosdisk;
		struct
		{
		    int		unit;
		    void	*data;
		} bioscd;
		struct 
		{
		    int		unit;		/* XXX net layer lives over these? */
		} netif;
    } d_kind;
};
```

### Code

```c
/*
 * Set the 'current device' by (if possible) recovering the boot device as 
 * supplied by the initial bootstrap.
 *
 * XXX should be extended for netbooting.
 */
static void
extract_currdev(void)
{
    struct i386_devdesc	new_currdev;
    int			major, biosdev = -1;

    /* Assume we are booting from a BIOS disk by default */
    new_currdev.d_dev = &biosdisk;

    /* new-style boot loaders such as pxeldr and cdldr */
    if (kargs->bootinfo == NULL) {
        if ((kargs->bootflags & KARGS_FLAGS_CD) != 0) {
	    /* we are booting from a CD with cdboot */
	    new_currdev.d_dev = &bioscd;
	    new_currdev.d_kind.bioscd.unit = bc_bios2unit(initial_bootdev);
	} else if ((kargs->bootflags & KARGS_FLAGS_PXE) != 0) {
	    /* we are booting from pxeldr */
	    new_currdev.d_dev = &pxedisk;
	    new_currdev.d_kind.netif.unit = 0;
	} else {
	    /* we don't know what our boot device is */
	    new_currdev.d_kind.biosdisk.slice = -1;
	    new_currdev.d_kind.biosdisk.partition = 0;
	    biosdev = -1;
	}
    } else if ((initial_bootdev & B_MAGICMASK) != B_DEVMAGIC) {
	/* The passed-in boot device is bad */
	new_currdev.d_kind.biosdisk.slice = -1;
	new_currdev.d_kind.biosdisk.partition = 0;
	biosdev = -1;
    } else {
	new_currdev.d_kind.biosdisk.slice = (B_ADAPTOR(initial_bootdev) << 4) +
					     B_CONTROLLER(initial_bootdev) - 1;
	new_currdev.d_kind.biosdisk.partition = B_PARTITION(initial_bootdev);
	biosdev = initial_bootinfo->bi_bios_dev;
	major = B_TYPE(initial_bootdev);

	/*
	 * If we are booted by an old bootstrap, we have to guess at the BIOS
	 * unit number.  We will loose if there is more than one disk type
	 * and we are not booting from the lowest-numbered disk type 
	 * (ie. SCSI when IDE also exists).
	 */
	if ((biosdev == 0) && (B_TYPE(initial_bootdev) != 2))	/* biosdev doesn't match major */
	    biosdev = 0x80 + B_UNIT(initial_bootdev);		/* assume harddisk */
    }
    new_currdev.d_type = new_currdev.d_dev->dv_type;
    
    /*
     * If we are booting off of a BIOS disk and we didn't succeed in determining
     * which one we booted off of, just use disk0: as a reasonable default.
     */
    if ((new_currdev.d_type == biosdisk.dv_type) &&
	((new_currdev.d_kind.biosdisk.unit = bd_bios2unit(biosdev)) == -1)) {
	printf("Can't work out which disk we are booting from.\n"
	       "Guessed BIOS device 0x%x not found by probes, defaulting to disk0:\n", biosdev);
	new_currdev.d_kind.biosdisk.unit = 0;
    }
    env_setenv("currdev", EV_VOLATILE, i386_fmtdev(&new_currdev),
	       i386_setcurrdev, env_nounset);
    env_setenv("loaddev", EV_VOLATILE, i386_fmtdev(&new_currdev), env_noset,
	       env_nounset);
}
```

## _main.c_: Obtaining the BIOS Segment Map

```c
void
bios_getsmap(void)
{
	int n;

	n = 0;
	smaplen = 0;
	/* Count up segments in system memory map */
	v86.ebx = 0;
	do {
		v86.ctl = V86_FLAGS;
		v86.addr = 0x15;		/* int 0x15 function 0xe820*/
		v86.eax = 0xe820;
		v86.ecx = sizeof(struct smap);
		v86.edx = SMAPSIG;
		v86.es = VTOPSEG(&smap);
		v86.edi = VTOPOFF(&smap);
		v86int();
		if ((v86.efl & 1) || (v86.eax != SMAPSIG))
			break;
		n++;
	} while (v86.ebx != 0);
	if (n == 0)
		return;
	n += 10;	/* spare room */
	smapbase = malloc(n * sizeof(*smapbase));

	/* Save system memory map */
	v86.ebx = 0;
	do {
		v86.ctl = V86_FLAGS;
		v86.addr = 0x15;		/* int 0x15 function 0xe820*/
		v86.eax = 0xe820;
		v86.ecx = sizeof(struct smap);
		v86.edx = SMAPSIG;
		v86.es = VTOPSEG(&smapbase[smaplen]);
		v86.edi = VTOPOFF(&smapbase[smaplen]);
		v86int();
		smaplen++;
		if ((v86.efl & 1) || (v86.eax != SMAPSIG))
			break;
	} while (v86.ebx != 0 && smaplen < n);
}
```

## _main.c_: Setting the Architecture Switch

### _archsw_ Structure

```c
/* 
 * The intention of the architecture switch is to provide a convenient
 * encapsulation of the interface between the bootstrap MI and MD code.
 * MD code may selectively populate the switch at runtime based on the
 * actual configuration of the target system.
 */
struct arch_switch
{
    /* Automatically load modules as required by detected hardware */
    int		(*arch_autoload)(void);
    /* Locate the device for (name), return pointer to tail in (*path) */
    int		(*arch_getdev)(void **dev, const char *name, const char **path);
    /* Copy from local address space to module address space, similar to bcopy() */
    ssize_t	(*arch_copyin)(const void *src, vm_offset_t dest,
			       const size_t len);
    /* Copy to local address space from module address space, similar to bcopy() */
    ssize_t	(*arch_copyout)(const vm_offset_t src, void *dest,
				const size_t len);
    /* Read from file to module address space, same semantics as read() */
    ssize_t	(*arch_readin)(const int fd, vm_offset_t dest,
			       const size_t len);
    /* Perform ISA byte port I/O (only for systems with ISA) */
    int		(*arch_isainb)(int port);
    void	(*arch_isaoutb)(int port, int value);
};
extern struct arch_switch archsw;
```

### Code

```c
    archsw.arch_autoload = i386_autoload;
    archsw.arch_getdev = i386_getdev;
    archsw.arch_copyin = i386_copyin;
    archsw.arch_copyout = i386_copyout;
    archsw.arch_readin = i386_readin;
    archsw.arch_isainb = isa_inb;
    archsw.arch_isaoutb = isa_outb;
```

## _interp.c_: Loading the Kernel

```c
/*
 * Interactive mode
 */
void
interact(void)
{
    char	input[256];			/* big enough? */
#ifndef BOOT_FORTH
    int		argc;
    char	**argv;
#endif

#ifdef BOOT_FORTH
    bf_init();
#endif

    /*
     * Read our default configuration
	 *  According to /boot/i386/forth/loader.rc, autoboot is going to be
	 *  automatic assuming that we have not made any modifications to the
	 *  default configuration. Hence, we autoboot after loading in the file.
     */
    if(include("/boot/loader.rc")!=CMD_OK)
		include("/boot/boot.conf");
    printf("\n");
    /*
     * Before interacting, we might want to autoboot.
     */
    autoboot_maybe();
	.
	.
	.
}
``` 
