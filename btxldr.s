#
# Copyright (c) 1998 Robert Nordier
# All rights reserved.
#
# Redistribution and use in source and binary forms are freely
# permitted provided that the above copyright notice and this
# paragraph and the following disclaimer are duplicated in all
# such forms.
#
# This software is provided "AS IS" and without any express or
# implied warranties, including, without limitation, the implied
# warranties of merchantability and fitness for a particular
# purpose.
#

# $FreeBSD$

#
# Prototype BTX loader program, written in a couple of hours.  The
# real thing should probably be more flexible, and in C.
#

#
# Memory locations.
#
		.set MEM_STUB,0x600		# Real mode stub
		.set MEM_ESP,0x1000		# New stack pointer
		.set MEM_TBL,0x5000		# BTX page tables
		.set MEM_ENTRY,0x9010		# BTX entry point
		.set MEM_DATA,start+0x1000	# Data segment
#
# Segment selectors.
#
		.set SEL_SCODE,0x8		# 4GB code
		.set SEL_SDATA,0x10		# 4GB data
		.set SEL_RCODE,0x18		# 64K code
		.set SEL_RDATA,0x20		# 64K data
#
# Paging constants.
#
		.set PAG_SIZ,0x1000		# Page size
		.set PAG_ENT,0x4		# Page entry size
#
# Screen constants.
#
		.set SCR_MAT,0x7		# Mode/attribute
		.set SCR_COL,0x50		# Columns per row
		.set SCR_ROW,0x19		# Rows per screen
#
# BIOS Data Area locations.
#
		.set BDA_MEM,0x413		# Free memory
		.set BDA_SCR,0x449		# Video mode
		.set BDA_POS,0x450		# Cursor position
#
# Required by aout gas inadequacy.
#
		.set SIZ_STUB,0x1a		# Size of stub
#
# We expect to be loaded by boot2 at the origin defined in ./Makefile.
#
		.globl start
#
# BTX program loader for ELF clients.
#
start:
		cld
		movl $m_logo,%esi		# %esi = "\nBTX Loader 1.00 "
		call putstr
		movzwl BDA_MEM,%eax		# %eax = 0x413, BIOS ext. mem location
		shll $0xa,%eax			# Convert free ext. mem to bytes
		movl %eax,%ebp			# %ebp points to base of client stack
ifdef(`BTXLDR_VERBOSE',`
		movl $m_mem,%esi		# %esi = Starting in protected mode (base mem=\0)\n"
		call hexout
		call putstr				# Print available user memory
')
		lgdt gdtdesc			# Load new GDT

#
# Relocate caller's arguments.
#

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
		movl $0x48,%ecx 		# Set count to 56 bytes
		subl %ecx,%ebp			# Decrement user stack (0xa0000 - 0x48)
		movl 0x18(%esp,1),%esi	# %esi = VTOP(&bootinfo)
		cmpl $0x0, %esi
		je start_null_bi		# Jump if ptr is NULL
		movl %ebp,%edi			# %edi = base of allocated stack memory
		rep
		movsb					# Relocate bootinfo args to the stack
								#  at 0x9ffb8
		movl %ebp,0x18(%esp,1)	# Update &bootinfo
ifdef(`BTXLDR_VERBOSE',`
		movl $m_rel_bi,%esi		# Display
		movl %ebp,%eax			#  bootinfo
		call hexout			#  relocation
		call putstr			#  message
')
start_null_bi:
		movl $0x18,%ecx 		# Set count to 24 bytes
		subl %ecx,%ebp			# Decrement client stack
		leal 0x4(%esp,1),%esi	# %esi = base of __exec arguments
								# (skipping VTOP(&bootinfo))
		movl %ebp,%edi			# Base of allocated stack memory
		rep
		movsb					# Relocate __exec's arguments
ifdef(`BTXLDR_VERBOSE',`
		movl $m_rel_args,%esi		# Display
		movl %ebp,%eax			#  argument
		call hexout			#  relocation
		call putstr			#  message
')
#
# Set up BTX kernel.
#

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
		incl %edi						# Incr %edi to obtain its page number
		shll $0x2,%edi					# Multiply page number by 4 to obtain
										# offset
		addl $MEM_TBL,%edi				# Add offset to base of page tables
										# at 0x5000
		pushl %edi						# Save load address
		movzwl 0xa(%ebx),%ecx			# %ecx = btx_textsz
ifdef(`BTXLDR_VERBOSE',`
		pushl %ecx			# Save image size
')
		rep
		movsb				# Relocate BTX kernel to data segment
		movl %esi,%ebx		# %esi = base of BTX kernel
ifdef(`BTXLDR_VERBOSE',`
		movl $m_rel_btx,%esi		# Restore
		popl %eax			#  parameters
		call hexout			#  and
')
		popl %ebp			# %ebp = load address
ifdef(`BTXLDR_VERBOSE',`
		movl %ebp,%eax			#  the
		call hexout			#  relocation
		call putstr			#  message
')
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
		cmpl $0x464c457f,(%ebx) 	# ELF magic number?
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
		movl 0x1c(%ebx),%edx		# Get e_phoff
		addl %ebx,%edx			# To pointer
		movzwl 0x2c(%ebx),%ecx		# Get e_phnum
start.4:	cmpl $0x1,(%edx)		# Is p_type PT_LOAD?
		jne start.6			# No
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
		movl 0x4(%edx),%esi		# Get p_offset
		addl %ebx,%esi			#  as pointer
		movl 0x8(%edx),%edi		# Get p_vaddr
		addl %ebp,%edi			#  as pointer
		movl 0x10(%edx),%ecx		# Get p_filesz
		rep				# Set up
		movsb				#  segment
		movl 0x14(%edx),%ecx		# Any bytes
		subl 0x10(%edx),%ecx		#  to zero?
		jz start.5			# No
		xorb %al,%al			# Then
		rep				#  zero
		stosb				#  them
start.5:	popl %ecx			# Restore
		popl %edi			#  working
		popl %esi			#  registers
		decl %edi			# Segments to do
		je start.7			# If none
start.6:	addl $0x20,%edx 		# To next entry
		loop start.4			# Till done
start.7:
ifdef(`BTXLDR_VERBOSE',`
		movl $m_done,%esi		# Display done
		call putstr			#  message
')
		movl $start.8,%esi		# Real mode stub
		movl $MEM_STUB,%edi		# Destination
		movl $start.9-start.8,%ecx	# Size
		rep				# Relocate
		movsb				#  it
		ljmp $SEL_RCODE,$MEM_STUB	# To 16-bit code
		.code16
start.8:	xorw %ax,%ax			# Data
		movb $SEL_RDATA,%al		#  selector
		movw %ax,%ss			# Reload SS
		movw %ax,%ds			# Reset
		movw %ax,%es			#  other
		movw %ax,%fs			#  segment
		movw %ax,%gs			#  limits
		movl %cr0,%eax			# Switch to
		decw %ax			#  real
		movl %eax,%cr0			#  mode
		ljmp $0,$MEM_ENTRY		# Jump to BTX entry point
start.9:
		.code32
#
# Output message [ESI] followed by EAX in hex.
#
hexout: 	pushl %eax			# Save
		call putstr			# Display message
		popl %eax			# Restore
		pushl %esi			# Save
		pushl %edi			#  caller's
		movl $buf,%edi			# Buffer
		pushl %edi			# Save
		call hex32			# To hex
		xorb %al,%al			# Terminate
		stosb				#  string
		popl %esi			# Restore
hexout.1:	lodsb				# Get a char
		cmpb $'0',%al			# Leading zero?
		je hexout.1			# Yes
		testb %al,%al			# End of string?
		jne hexout.2			# No
		decl %esi			# Undo
hexout.2:	decl %esi			# Adjust for inc
		call putstr			# Display hex
		popl %edi			# Restore
		popl %esi			#  caller's
		ret				# To caller
#
# Output zero-terminated string [ESI] to the console.
#
putstr.0:	call putchr			# Output char
putstr: 	lodsb				# Load char
		testb %al,%al			# End of string?
		jne putstr.0			# No
		ret				# To caller
#
# Output character AL to the console.
#
putchr: 	pusha				# Save
		xorl %ecx,%ecx			# Zero for loops
		movb $SCR_MAT,%ah		# Mode/attribute
		movl $BDA_POS,%ebx		# BDA pointer
		movw (%ebx),%dx 		# Cursor position
		movl $0xb8000,%edi		# Regen buffer (color)
		cmpb %ah,BDA_SCR-BDA_POS(%ebx)	# Mono mode?
		jne putchr.1			# No
		xorw %di,%di			# Regen buffer (mono)
putchr.1:	cmpb $0xa,%al			# New line?
		je putchr.2			# Yes
		xchgl %eax,%ecx 		# Save char
		movb $SCR_COL,%al		# Columns per row
		mulb %dh			#  * row position
		addb %dl,%al			#  + column
		adcb $0x0,%ah			#  position
		shll %eax			#  * 2
		xchgl %eax,%ecx 		# Swap char, offset
		movw %ax,(%edi,%ecx,1)		# Write attr:char
		incl %edx			# Bump cursor
		cmpb $SCR_COL,%dl		# Beyond row?
		jb putchr.3			# No
putchr.2:	xorb %dl,%dl			# Zero column
		incb %dh			# Bump row
putchr.3:	cmpb $SCR_ROW,%dh		# Beyond screen?
		jb putchr.4			# No
		leal 2*SCR_COL(%edi),%esi	# New top line
		movw $(SCR_ROW-1)*SCR_COL/2,%cx # Words to move
		rep				# Scroll
		movsl				#  screen
		movb $' ',%al			# Space
		movb $SCR_COL,%cl		# Columns to clear
		rep				# Clear
		stosw				#  line
		movb $SCR_ROW-1,%dh		# Bottom line
putchr.4:	movw %dx,(%ebx) 		# Update position
		popa				# Restore
		ret				# To caller
#
# Convert EAX, AX, or AL to hex, saving the result to [EDI].
#
hex32:		pushl %eax			# Save
		shrl $0x10,%eax 		# Do upper
		call hex16			#  16
		popl %eax			# Restore
hex16:		call hex16.1			# Do upper 8
hex16.1:	xchgb %ah,%al			# Save/restore
hex8:		pushl %eax			# Save
		shrb $0x4,%al			# Do upper
		call hex8.1			#  4
		popl %eax			# Restore
hex8.1: 	andb $0xf,%al			# Get lower 4
		cmpb $0xa,%al			# Convert
		sbbb $0x69,%al			#  to hex
		das				#  digit
		orb $0x20,%al			# To lower case
		stosb				# Save char
		ret				# (Recursive)

		.data
		.p2align 4
#
# Global descriptor table.
#
gdt:		.word 0x0,0x0,0x0,0x0		# Null entry
		.word 0xffff,0x0,0x9a00,0xcf	# SEL_SCODE
		.word 0xffff,0x0,0x9200,0xcf	# SEL_SDATA
		.word 0xffff,0x0,0x9a00,0x0	# SEL_RCODE
		.word 0xffff,0x0,0x9200,0x0	# SEL_RDATA
gdt.1:
gdtdesc:	.word gdt.1-gdt-1		# Limit
		.long gdt			# Base
#
# Messages.
#
m_logo: 	.asciz " \nBTX loader 1.00  "
m_vers: 	.asciz "BTX version is \0\n"
e_fmt:		.asciz "Error: Client format not supported\n"
ifdef(`BTXLDR_VERBOSE',`
m_mem:		.asciz "Starting in protected mode (base mem=\0)\n"
m_esp:		.asciz "Arguments passed (esp=\0):\n"
m_args: 	.asciz"<howto="
		.asciz" bootdev="
		.asciz" junk="
		.asciz" "
		.asciz" "
		.asciz" bootinfo=\0>\n"
m_rel_bi:	.asciz "Relocated bootinfo (size=48) to \0\n"
m_rel_args:	.asciz "Relocated arguments (size=18) to \0\n"
m_rel_btx:	.asciz "Relocated kernel (size=\0) to \0\n"
m_base: 	.asciz "Client base address is \0\n"
m_elf:		.asciz "Client format is ELF\n"
m_segs: 	.asciz "text segment: offset="
		.asciz " vaddr="
		.asciz " filesz="
		.asciz " memsz=\0\n"
		.asciz "data segment: offset="
		.asciz " vaddr="
		.asciz " filesz="
		.asciz " memsz=\0\n"
m_done: 	.asciz "Loading complete\n"
')
#
# Uninitialized data area.
#
buf:						# Scratch buffer
