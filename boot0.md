# Walkthrough of *boot0.S*

## Overview

1. Introduction
2. Master Boot Record (MBR)
3. Makefile
4. Memory Maps
5. Relocating Boot0
6. Reading the Partition Table
7. Check for the Next Drive
8. Booting the Partition

## Introduction

The primary purpose of boot0.S is to read the partition table and let the user
choose which partition to boot from. The code is pretty straightforward with
accompanying visual aids.

## Master Boot Record (MBR)

### FreeBSD Master Boot Record

| Offset | Length (bytes) | Description |
|:------:|:--------------:|:------------|
| 0x00 | 446 | Bootstrap code area |
| 0x1BE | 16 | Partition Record 1 |
| 0x1CE | 16 | Partition Record 2 |
| 0x1DE | 16 | Partition Record 3 | 
| 0x1EE | 16 | Partition Record 4 |
| 0x1FE | 2  | Magic Number (0xaa55) |

### Partition Record

| Offset | Length (bytes) | Description |
|:------:|:--------------:|:------------|
| 0x0 | 1 | Filesystem type/Drive Number |
| 0x1 | 1 | Bootable Flag; 0x80 = Active |
| 0x2 | 6 | CHS Descriptor |
| 0x8 | 8 | LBA Descriptor |

## Makefile

```txt
# $FreeBSD$

PROG=		boot0
NOMAN=
STRIP=
BINDIR?=	/boot
BINMODE=	444

M4?=	m4

# The default set of flags compiled into boot0.  This enables update (writing
# the modified boot0 back to disk after running so that the selection made is
# saved), packet mode (detect and use the BIOS EDD extensions if we try to
# boot past the 1024 cylinder liimt), and booting from all valid slices.
BOOT_BOOT0_FLAGS?=	0xf

# The number of timer ticks to wait for a keypress before assuming the default
# selection.  Since there are 18.2 ticks per second, the default value of
# 0xb6 (182d) corresponds to 10 seconds.
BOOT_BOOT0_TICKS?=	0xb6

# The base address that we the boot0 code to to run it.  Don't change this
# unless you are glutton for punishment.
BOOT_BOOT0_ORG?=	0x600

boot0: boot0.o
	${LD} -N -e start -Ttext ${BOOT_BOOT0_ORG} -o boot0.out boot0.o
	objcopy -S -O binary boot0.out ${.TARGET}

boot0.o: boot0.s
	${AS} ${AFLAGS} --defsym FLAGS=${BOOT_BOOT0_FLAGS} \
		--defsym TICKS=${BOOT_BOOT0_TICKS} ${.IMPSRC} -o ${.TARGET}

CLEANFILES+= boot0.out boot0.o

.include <bsd.prog.mk>
```

## Memory Maps

### Memory Map (Boot0 Stage)

```txt
____________________________________________________ 0x600
|                                                  |
|                 Relocated Boot0                  |
|__________________________________________________| 0x800
|                                                  |
|               Fake Partition Entry               |
|__________________________________________________| 0x80f
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|--------------------------------------------------|
|                                                  | 0x7c00
|                   MBR/Boot0                      |
|__________________________________________________|
```

### Boot0 Sector

```txt
       ____________________________________________________ 0x600
       |                                                  |
       |                                                  |
       |                                                  |
       |                                                  |
       |                   Boot Code                      |
       |                                                  |
       |                                                  |
-0x48  |__________________________________________________| 0x7b8
       |                                                  |
       |                 Next Drive (_NXTDRV)             |
-0x47  |__________________________________________________| 0x7b9
       |                                                  |
       |                Default Option (_OPT)             |
-0x46  |__________________________________________________| 0x7ba
       |                                                  |
       |             Set Drive Option (_STDRV)            |
       |                 Default = 0x0                    |
-0x45  |__________________________________________________| 0x7bb
       |                                                  |
       |                 Flags (_FLAGS)                   |
       |                 Default = 0xf                    |
-0x44  |__________________________________________________| 0x7bc
       |                                                  |
       |              Timeout Ticks (_TICKS)              |
       |                 Default = 0xb6                   |
       |__________________________________________________| 0x800
       |                                                  |
       |               Fake Partition (_FAKE)             |
       |__________________________________________________| 0x80f
```

## Relocating Boot0

```c
start:
		cld
		xorw %ax,%ax
		movw %ax,%es
		movw %ax,%ds
		movw %ax,%ss
		movw $LOAD,%sp			# $LOAD = 0x7c00
	
#
# Copy this code to the address it was linked for
#
		movw %sp,%si			# %si = 0x7c00
		movw $start,%di			# %di = 0x600
		movw $0x100,%cx			# Word count (256 words == 512 bytes)
		rep	
		movsw					# Relocates boot sector to 0x600 - 0x800
#
# Set address for variable space beyond code, and clear it.
# Notice that this is also used to point to the values embedded in the block,
# by using negative offsets.
#
		movw %di,%bp			# %bp = 0x800
		movb $0x8,%cl			# Words to clear
		rep
		stosw					# Clear 16 bytes
								# Fake Partition Table: 0x800 - 0x810 (exclusive upper bound)
								# %di = 0x810
#
# Relocate to the new copy of the code.
#
		incb -0xe(%di)			# -0xe(%di) = 0x802
								# Corresponds to fake partition entry's CHS offset
								# Set the sector to 1, which is MBR
		jmp main-LOAD+ORIGIN	# Since main is located relative to LOAD, we
								# subtract LOAD and add ORIGIN to obtain its new value
```

## Reading the Partition Table

After we determining the drive number and saving it to the fake partition entry,
 we begin reading the partition table and printing its data to the console.

```c
#
# Start out with a pointer to the 4th byte of the first table entry
# so that after 4 iterations it's beyond the end of the sector.
# and beyond a 256 byte boundary and has overflowed 8 bits (see next comment).
# (remember that the table starts 2 bytes earlier than you would expect
# as the bootable flag is after it in the block)
#
		movw $(partbl+0x4),%bx		# Partition table (+4)
		xorw %dx,%dx				# Item number
#
# Loop around on the partition table, printing values until we
# pass a 256 byte boundary. The end of loop test is at main.5.
#
main.3:
		movb %ch,-0x4(%bx)		# Zero active flag (ch == 0)
								# Clear active flag in entry
		btw %dx,_FLAGS(%bp)		# Entry enabled?
								# Check bit number %dx in _FLAGS(%bp) and set its value
								# to Carry Flag (CF)
		jnc main.5				# If CF is not set, we jump to main.5 
#
# If any of the entries in the table are
# the same as the 'type' in the slice table entry,
# then this is an empty or non bootable partition. Skip it.
#
		movb (%bx),%al			# Load type
		movw $tables,%di		# Lookup tables
								# Contains three entries of invalid boot types/partitions
								# Contains eleven entries that are valid/known

		movb $TBL0SZ,%cl		# %cl = 3
		repne					# Repeat while not equal
		scasb					# Scans %di and compares its byte value with %al
		je main.5				# Jump if Load Type is invalid (string matched)
#
# Now scan the table of known types
#
		movb $TBL1SZ,%cl		# %cl = 11
		repne					# Repeat while not equal
		scasb					# Scans %di and compares its byte value with %al
		jne main.4				# Jump if Load type is not known (string mismatch)
#
# If it matches get the matching element in the
# next array. if it doesn't, we are already
# pointing at its first element which points to a "?".
#
		addw $TBL1SZ,%di		# Add TBL1SZ to obtain the entry in the following
								# table with a string description of Load Type

main.4:
		movb (%di),%cl			# Partition
		addw %cx,%di			#  description
		callw putx				# Display it
main.5:
		incw %dx			# Next item 
		addb $0x10,%bl		# Next entry
		jnc main.3			# Till done
``` 

## Check for the Next Drive

```c
#
# Add one to the drive number and check it is valid, 
#
		popw %ax
		subb $0x80-0x1,%al
		cmpb NHRDRV,%al		# Sets flags for NHRDRV - %al
		jb main.6			# Jump if NHRDRV > %al
							# Else we set drive number to 0

# If not then if there is only one drive,
# Don't display drive as an option.
#
		decw %ax			# Already drive 0?
		jz main.7			# Yes
# If it was illegal or we cycled through them,
# then go back to drive 0.
#
		xorb %al,%al			# Drive 0

#
# Whatever drive we selected, make it an ascii digit and save it back
# to the "next drive" location in the loaded block in case we
# want to save it for next time.
# This also is part of the printed drive string so add 0x80 to indicate
# end of string.
#
main.6:
		addb $'0'|0x80,%al		# Save next
		movb %al,_NXTDRV(%bp)		#  drive number
		movw $drive,%di			# Display
		callw putx			#  item
```

## Booting the Partition

```c
#
# We have a selection.
# but if it's a bad selection go back to complain.
# The bits in MNUOPT were set when the options were printed.
# Anything not printed is not an option.
#
main.12:
		cbtw						# Sign extend %al to %ax
		btw %ax,_MNUOPT(%bp)		# Check if the partition option was printed
		jnc main.10					# Jump if option was not printed

#
# Save the info in the original tables
# for rewriting to the disk.
#
		movb %al,_OPT(%bp)		# Save default option
		movw $FAKE,%si
		movb (%si),%dl			# Save drive number
		movw %si,%bx			# Fake Partition set to read
		cmpb $0x4,%al
		pushf
		je main.13				# Jump if F5 was pressed

		shlb $0x4,%al			# Convert selection to partition index
		addw $partbl,%ax		# Select chosen partitition
		xchgw %bx,%ax	 		# Write $FAKE to the partition table
		movb $0x80,(%bx)		# Set flag to active
#
# If not asked to do a write-back (flags 0x40) don't do one.
#
main.13:
		pushw %bx					# Push Fake partition address
		testb $0x40,_FLAGS(%bp)
		jnz main.14					# Jump if write-back is not set

		movw $start,%bx				# Data buffer for BIOS call
		movb $0x3,%ah
		callw intx13				# Check routine at bottom of code block

main.14:
		popw %si			# Restore %bx to fake partition
		popf				# Restore flags for the test on the selection

#
# If going to next drive, replace drive with selected one.
# Remember to un-ascii it. Hey 0x80 is already set, cool!
#
		jne main.15				# Jump if F5 wasn't pressed
		movb _NXTDRV(%bp),%dl	# Obtain next drive number
		subb $'0',%dl			# Convert ASCII to raw number

# 
# load selected bootsector to the LOAD location in RAM.
# If it fails to read or isn't marked bootable, treat it
# as a bad selection.
# XXX what does %si carry?
#
main.15:
		movw $LOAD,%bx				# %bx = 0x7c00
		movb $0x2,%ah				# Read sector
		callw intx13				#  from disk
		jc main.10					# Jump if error

		cmpw $MAGIC,0x1fe(%bx)		# Is the magic number 0xaa55
		jne main.10					# Jump if sector isn't bootable
		pushw %si					# Save address of fake partition entry
		movw $crlf,%si				# Leave some
		callw puts					#  space
		popw %si					# Restore
		jmp *%bx					# Invoke bootstrap

# BIOS Call: INT 0x13, AH = 0x2
#  Read disk sectors into memory
#
#  AL = number of sectors to read
#  CH = low eight bits of cylinder number
#  CL = sector number 1-63 (bits 0-5)
#       high two bits of cylinder (bits 6-7)
#  DH = head number
#  DL = drive number (bit 7 set for hard disk)
#  ES:BX -> data buffer
#
#  Return:
#    CF set on error
#    CF clear if successful
#    AH = status
#    AL = number of sectors transferred
#

# BIOS Call: INT 0x13, AH = 0x3
#  Write disk sectors.
#
#  AL = number of sectors to write
#  CH = low eight bits of cylinder number
#  CL = sector number 1-63 (bits 0-5)
#       high two bits of cylinder (bits 6-7)
#  DH = head number
#  DL = drive number (bit 7 set for hard disk)
#  ES:BX -> data buffer
#
#  Return:
#    CF set on error
#    CF clear if successful
#    AH = status
#    AL = number of sectors transferred
#

intx13:
		movb 0x1(%si),%dh		# Load head
		movw 0x2(%si),%cx		# Load cylinder:sector
		movb $0x1,%al			# Sector count
		pushw %si			# Save
		movw %sp,%di			# Save
		testb $0x80,_FLAGS(%bp)		# Use packet interface?
		jz intx13.1			# No
		pushl $0x0			# Set the
		pushl 0x8(%si)			# LBA address
		pushw %es			# Set the transfer
		pushw %bx			#  buffer address
		push  $0x1			# Block count
		push  $0x10			# Packet size
		movw %sp,%si			# Packet pointer
		decw %ax			# Verify off
		orb $0x40,%ah			# Use disk packet

intx13.1:
		int $0x13			# BIOS: Disk I/O
		movw %di,%sp			# Restore
		popw %si			# Restore
		retw				# To caller
```
