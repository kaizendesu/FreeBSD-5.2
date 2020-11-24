# Walkthrough of FreeBSD 5.2's Boot Process

## Contents

1. Code Flow
2. General Overview (Visual Aids)
3. General Overview (Code)
4. Allocating and Filling KPT Entries

## Code Flow

The first '+' means that I have read the code or I have a general idea of what it does.
The second '+' means that I have read the code closely and documented it as it relates to bootup.
The third '+' means that I have added it to this markdown document for reference.

```txt
File: boot0.s
	start()					+++

File: boot1.s
	main()					+++
File: btx.s
	init()					+++

File: boot2.c
	main()					+++

File: btxldr.s
	start()					+++

File: loader.c
	main()					+++

File: locore.S
	recover_bootinfo		++-
	identify_cpu			++-
	create_pagetables		++-

File: machdep.c
	init386()				+--
	mutex_init()			+--
	cninit()				+--
	vm86_initialize()		+--
	getmemsize()			+--
		pmap_bootstrap()	+--
	msgbufinit()			+--

File: init_main.c
	mi_startup				+--
```

## General Overview (Visual Aids)

This section follows the state of memory as the kernel boot process takes place.
It was used primarily to understand the memory references in the assembly code and to
understand precisely what the code was doing.

It is recommended that you use this to review the boot process if it has been
a while since you've looked at it.

### Memory Map: _boot0_ Stage

```txt
____________________________________________________ 0x600
|                                                  |
|                                                  |
|                                                  |
|                                                  |
|                   Boot Code                      |
|                                                  |
|                                                  |
|__________________________________________________| 0x7b8 (-ox48)
|                                                  |
|                 Next Drive (_NXTDRV)             |
|__________________________________________________| 0x7b9 (-0x47)
|                                                  |
|                Default Option (_OPT)             |
|__________________________________________________| 0x7ba (-0x46)
|                                                  |
|             Set Drive Option (_STDRV)            |
|                 Default = 0x0                    |
|__________________________________________________| 0x7bb (-0x45)
|                                                  |
|                 Flags (_FLAGS)                   |
|                 Default = 0xf                    |
|__________________________________________________| 0x7bc (-0x44)
|                                                  |
|              Timeout Ticks (_TICKS)              |
|                 Default = 0xb6                   |
|__________________________________________________| 0x800
|                                                  |
|               Fake Partition (_FAKE)             |
|__________________________________________________| 0x80f
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|--------------------------------------------------|
|                                                  | 0x7c00
|                   MBR/Boot0                      |
|__________________________________________________|
```

### Memory Map: Transferring to _boot1_

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
|                    Boot1                         |
|__________________________________________________|
```

### Memory Map: Jumping to _btx_

```txt
_______________________________________________ 0x600
|                                             |
|           Relocated Boot0/MBR               |
|_____________________________________________| 0x700
|                                             |
|            Relocated Boot1                  |
|_____________________________________________| 0x900
|                                             |
|               Arguments                     |
|_____________________________________________|
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|---------------------------------------------| 0x7c00
|                                             |
|                 Boot1                       |
|_____________________________________________|
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|---------------------------------------------| 0x8cec
|                                             |
|                 Boot1                       |
|_____________________________________________| 0x8eec
|                                             |
|               Disklabel                     |
|_____________________________________________| 0x9000
|                                             |
|               BTX Server                    |
|_____________________________________________| 
|                                             |
|                 Boot2                       |
|_____________________________________________| 0xacec
|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
|---------------------------------------------| 0xc000
|                                             |
|               BTX Client                    |
|_____________________________________________| 0x10000
```

## Memory Map: Transferring to _boot2.c_ Code

```c
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
```

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
|                    Loader                       |
|_________________________________________________|
```

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

### _loader.c_: Supervisor and User Stack

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

### General Overview (Code)

```c
#
# Initialise segments and registers to known values.
# segments start at 0.
# The stack is immediately below the address we were loaded to.
#
start:
		cld						# String ops inc
		xorw %ax,%ax			# Zero
		movw %ax,%es			# Address
		movw %ax,%ds			#  data
		movw %ax,%ss			# Set up
		movw $LOAD,%sp			# %sp = 0x7C00
	
#
# Copy this code to the address it was linked for
#
		movw %sp,%si			# %si = 0x7C00
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
		incb -0xe(%di)			# -0xe(%di) = 0x800
								# Memory address 0x800 is set to 1 for CHS Addressing
		jmp main-LOAD+ORIGIN	# Since main is located relative to LOAD, we
								# subtract LOAD and add ORIGIN to obtain its new value
#
# Check what flags were loaded with us, specifically, Use a predefined Drive.
# If what the bios gives us is bad, use the '0' in the block instead, as well.
#
main:
		testb $0x20,_FLAGS(%bp)		# Is SETDRV set? (Not default)
		jnz main.1					# Jump here if _FLAGS(%bp) = 0x20
		testb %dl,%dl				# Drive number valid?
		js main.2					# Jump here if %dl has any value

main.1:
		movb _SETDRV(%bp),%dl		# Set the Drive number to use

#
# Whatever we decided to use, now store it into the fake
# partition entry that lives in the data space above us.
#
main.2:
		movb %dl,_FAKE(%bp)		# Save drive number
		callw putn				# To new line
		pushw %dx				# Save drive number
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
		jnc main.5				# No
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
		je main.5				# Yes
#
# Now scan the table of known types
#
		movb $TBL1SZ,%cl		# %cl = 11
		repne					# Repeat while not equal
		scasb					# Scans %di and compares its byte value with %al
		jne main.4				# No
#
# If it matches get the matching element in the
# next array. if it doesn't, we are already
# pointing at its first element which points to a "?".
#
		addw $TBL1SZ,%di		# Adjust
main.4:
		movb (%di),%cl			# Partition
		addw %cx,%di			#  description
		callw putx				# Display it
main.5:
		incw %dx			# Next item 
		addb $0x10,%bl		# Next entry
		jnc main.3			# Till done
#
# Passed a 256 byte boundary..
# table is finished.
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
#
# Now that we've printed the drive (if we needed to), display a prompt.
# Get ready for the input by noting the time.
#
main.7:
		movw $prompt,%si		# Move "Default:" string to %si
		callw putstr			# Print %si
		movb _OPT(%bp),%dl		# Display
		decw %si				#  default
		callw putkey			#  key

# BIOS Call: INT 0x1A, AH = 0x0
#  Gets the system time.
#
#   Return:
#     CX:DX = # of clock ticks since midnight
#     AL = midnight flag, =/= 0 if midnight passed since last read
#
		xorb %ah,%ah
		int $0x1a
		movw %dx,%di
		addw _TICKS(%bp),%di	#  timeout

# 
# Busy loop, looking for keystrokes but
# keeping one eye on the time.

main.8:

# BIOS Call: INT 0x16, AH = 0x1
#  Check for keystroke.
#
#   Return:
#     ZF = 0  keystroke available
#     AH = BIOS scan code
#     AL = ASCII character
#
		movb $0x1,%ah
		int $0x16
		jnz main.11		# Jump if ZF = 0 (keystroke available)
		xorb %ah,%ah
		int $0x1a
		cmpw %di,%dx
		jb main.8
#
# If timed out or defaulting, come here.
#
main.9:
		movb _OPT(%bp),%al		# Load default
		jmp main.12			# Join common code
#
# User's last try was bad, beep in displeasure.
# Since nothing was printed, just continue on as if the user
# hadn't done anything. This gives the effect of the user getting a beep 
# for all bad keystrokes but no action until either the timeout
# occurs or the user hits a good key.
#
main.10:
		movb $0x7,%al			# Signal
		callw putchr			#  error
#
# Get the keystroke.
#
main.11:
		xorb %ah,%ah			# BIOS: Get
		int $0x16			#  keypress
		movb %ah,%al			# Scan code
#
# If it's CR act as if timed out.
#
		cmpb $KEY_ENTER,%al		# Enter pressed?
		je main.9			# Yes
#
# Otherwise check if legal
# If not ask again.
#
		subb $KEY_F1,%al		# Less F1 scan code
		cmpb $0x4,%al			# F1..F5?
		jna main.12			# Yes
		subb $(KEY_1 - KEY_F1),%al	# Less #1 scan code
		cmpb $0x4,%al			# #1..#5?
		ja main.10			# No
#
# We have a selection.
# but if it's a bad selection go back to complain.
# The bits in MNUOPT were set when the options were printed.
# Anything not printed is not an option.
#
main.12:
		cbtw						# Option
		btw %ax,_MNUOPT(%bp)	 	#  enabled?
		jnc main.10					# No
#
# Save the info in the original tables
# for rewriting to the disk.
#
		movb %al,_OPT(%bp)		# Save option
		movw $FAKE,%si			# Partition for write
		movb (%si),%dl			# Drive number
		movw %si,%bx			# Partition for read
		cmpb $0x4,%al			# F5 pressed?
		pushf				# Save
		je main.13			# Yes
		shlb $0x4,%al			# Point to
		addw $partbl,%ax		#  selected
		xchgw %bx,%ax	 		#  partition
		movb $0x80,(%bx)		# Flag active
#
# If not asked to do a write-back (flags 0x40) don't do one.
#
main.13:
		pushw %bx			# Save
		testb $0x40,_FLAGS(%bp)		# No updates?
		jnz main.14			# Yes
		movw $start,%bx			# Data to write
		movb $0x3,%ah			# Write sector
		callw intx13			#  to disk
main.14:
		popw %si			# Restore
		popf				# Restore
#
# If going to next drive, replace drive with selected one.
# Remember to un-ascii it. Hey 0x80 is already set, cool!
#
		jne main.15			# If not F5
		movb _NXTDRV(%bp),%dl		# Next drive
		subb $'0',%dl			#  number
# 
# load selected bootsector to the LOAD location in RAM.
# If it fails to read or isn't marked bootable, treat it
# as a bad selection.
# XXX what does %si carry?
#
main.15:
		movw $LOAD,%bx			# Address for read
		movb $0x2,%ah			# Read sector
		callw intx13			#  from disk
		jc main.10				# If error
		cmpw $MAGIC,0x1fe(%bx)	# Bootable?
		jne main.10			# No
		pushw %si			# Save
		movw $crlf,%si		# Leave some
		callw puts			#  space
		popw %si			# Restore
		jmp *%bx			# Invoke bootstrap


start:	jmp main			// Start recognizably

// This is the start of a standard BIOS Parameter Block (BPB). Most bootable
// FAT disks have this at the start of their MBR. While normal BIOS's will
// work fine without this section, IBM's El Torito emulation "fixes" up the
// BPB by writing into the memory copy of the MBR. Rather than have data
// written into our xread routine, we'll define a BPB to work around it.
// The data marked with (T) indicates a field required for a ThinkPad to
// recognize the disk and (W) indicates fields written from IBM BIOS code.
// The use of the BPB is based on what OpenBSD and NetBSD implemented in
// their boot code but the required fields were determined by trial and error.
//
// Note: If additional space is needed in boot1, one solution would be to
// move the "prompt" message data (below) to replace the OEM ID.

		.org 0x03, 0x00
oemid:		.space 0x08, 0x00	// OEM ID

		.org 0x0b, 0x00
bpb:		.word   512		// sector size (T)
		.byte	0		// sectors/clustor
		.word	0		// reserved sectors
		.byte	0		// number of FATs
		.word	0		// root entries
		.word	0		// small sectors
		.byte	0		// media type (W)
		.word	0		// sectors/fat
		.word	18		// sectors per track (T)
		.word	2		// number of heads (T)
		.long	0		// hidden sectors (W)
		.long	0		// large sectors

		.org 0x24, 0x00
ebpb:		.byte	0		// BIOS physical drive number (W)

		.org 0x25,0x90
// 
// Trampoline used by boot2 to call read to read data from the disk via
// the BIOS.  Call with:
//
// %cx:%ax	- long    - LBA to read in
// %es:(%bx)	- caddr_t - buffer to read data into
// %dl		- byte    - drive to read from
// %dh		- byte    - num sectors to read
// 

xread:
		push %ss			// Address
		pop %ds				//  data
//
// Setup an EDD disk packet and pass it to read
// 
xread.1:					// Starting
		pushl $0x0			//  absolute
		push %cx			//  block
		push %ax			//  number
		push %es			// Address of
		push %bx			//  transfer buffer
		xor %ax,%ax			// Number of
		movb %dh,%al			//  blocks to
		push %ax			//  transfer
		push $0x10			// Size of packet
		mov %sp,%bp			// Packet pointer
		callw read			// Read from disk
		lea 0x10(%bp),%sp		// Clear stack
		lret				// To far caller
// 
// Load the rest of boot2 and BTX up, copy the parts to the right locations,
// and start it all up.
//

//
// Setup the segment registers to flat addressing (segment 0) and setup the
// stack to end just below the start of our code.
// 
main:
		cld				// String ops inc
		xor %cx,%cx			// Zero
		mov %cx,%es			// Address
		mov %cx,%ds			//  data
		mov %cx,%ss			// Set up
		mov $start,%sp		//  stack
//
// Relocate ourself to MEM_REL.  Since %cx == 0, the inc %ch sets
// %cx == 0x100.
// 
		mov %sp,%si			// Source = $start
		mov $MEM_REL,%di	// Destination = 0x700
		incb %ch			// Word count = 256 words (512 bytes)
		rep
		movsw				// Move boot1 to 0x700
//
// If we are on a hard drive, then load the MBR and look for the first
// FreeBSD slice.  We use the fake partition entry below that points to
// the MBR when we call nread.  The first pass looks for the first active
// FreeBSD slice.  The second pass looks for the first non-active FreeBSD
// slice if the first one fails.
// 
		mov $part4,%si		// %si = fake partition 
		cmpb $0x80,%dl		// Hard drive?
		jb main.4			// Jump if the drive number is not a hard drive
		movb $0x1,%dh		// Sector Count
		callw nread			// Read MBR into 0x8cec 

		mov $0x1,%cx	 			// Two passes
main.1:
		mov $MEM_BUF+PRT_OFF,%si	// %si = 0x8eaa
		movb $0x1,%dh				// Partition entry
main.2:
		cmpb $PRT_BSD,0x4(%si)		// Is BSD partition?
		jne main.3					// Jump if the partition isn't BSD
		jcxz main.5					// If second pass (cx = 0x0)
		testb $0x80,(%si)			// First Pass: is partition active?
		jnz main.5					// Jump if the partition is active
main.3:
		add $0x10,%si	 			// Next entry
		incb %dh					// Increment partition entry
		cmpb $0x1+PRT_NUM,%dh		// is partition entry < 0x5?
		jb main.2					// Jump if entry < 0x5
		dec %cx						// Decrement cx; start second pass
		jcxz main.1
//
// If we get here, we didn't find any FreeBSD slices at all, so print an
// error message and die.
// 
		mov $msg_part,%si		// Message
		jmp error				// Error
//
// Floppies use partition 0 of drive 0.
// 
main.4: 	xor %dx,%dx			// Partition:drive
//
// Ok, we have a slice and drive in %dx now, so use that to locate and load
// boot2.  %si references the start of the slice we are looking for, so go
// ahead and load up the first 16 sectors (boot1 + boot2) from that.  When
// we read it in, we conveniently use 0x8cec as our transfer buffer.  Thus,
// boot1 ends up at 0x8cec, and boot2 starts at 0x8cec + 0x200 = 0x8eec.
// The first part of boot2 is the disklabel, which is 0x114 bytes long.
// The second part is BTX, which is thus loaded into 0x9000, which is where
// it also runs from.  The boot2.bin binary starts right after the end of
// BTX, so we have to figure out where the start of it is and then move the
// binary to 0xc000.  Normally, BTX clients start at MEM_USR, or 0xa000, but
// when we use btxld to create boot2, we use an entry point of 0x2000.  That
// entry point is relative to MEM_USR; thus boot2.bin starts at 0xc000.
// 
main.5:
		mov %dx,MEM_ARG			// Move %dx to 0x900
		movb $NSECT,%dh			// Sector count: %dh = 0x10
		callw nread				// Read 16 sectors to 0x8cec
		mov $MEM_BTX,%bx		// %bx = 0x9000, the beginning of BTX
		mov 0xa(%bx),%si		// Set %si to BTX length
		add %bx,%si				//  %si = start of boot2.bin 

		mov $MEM_USR+SIZ_PAG*2,%di			// %di = 0xc000; Client page 2
		mov $MEM_BTX+(NSECT-1)*SIZ_SEC,%cx	// %cx = 0x920f

		sub %si,%cx					// Obtain size of client
		rep
		movsb						// Move client to 0xc000
		sub %di,%cx					// Byte count
		xorb %al,%al				// Zero assumed bss from
		rep							// the end of boot2.bin
		stosb						//  up to 0x10000
		callw seta20				// Enable A20
		jmp start+MEM_JMP-MEM_ORG	// Start BTX

#
# Initialization routine.
#
init:
		cli					# Disable interrupts
		xor %ax,%ax
		mov %ax,%ss
		mov $MEM_ESP0,%sp	# %sp = 0x1800; supervisor stack
		mov %ax,%es
		mov %ax,%ds
		pushl $0x2
		popfl				# Clear EFLAGS
#
# Initialize memory.
#
		mov $MEM_IDT,%di				# %di = 0x1e00
		mov $(MEM_ORG-MEM_IDT)/2,%cx	# %cx = 0x7200 ; Word Count
		push %di
		rep
		stosw							# Clear memory range 0x1e00 - 0x9000
		pop %di
#
# Create IDT.
#
		mov $idtctl,%si			# %si = IDT control string
init.1:
		lodsb					# Loads byte from %si to %al
		cbw						# Sign extend %al to %ax
		xchg %ax,%cx			# Set %cx = %ax
		jcxz init.4				# If done
		lodsb					# Loads P, DPL, and type flags to %al
		xchg %ax,%dx	 		# %dx = P:DPL:type flags
		lodsw					# Load control to %ax
		xchg %ax,%bx			# %bx = control
		lodsw					# Load handler offset
		mov $SEL_SCODE,%dh		# %dh = supervisor segment selector

init.2:
		shr %bx				# Handle this int?
		jnc init.3			# Jump if we don't handle the interrupt
		mov %ax,(%di)			# Set handler offset
		mov %dh,0x2(%di)		# Set selector
		mov %dl,0x5(%di)		# Set P:DPL:type flags
		add $0x4,%ax			# Next handler

init.3:
		lea 0x8(%di),%di	# Increment to next entry in IDT
		loop init.2			# Loop until %cx = 0
		jmp init.1			# Continue
#
# Initialize TSS.
#
init.4:
		movb $_ESP0H,TSS_ESP0+1(%di)	# Set ESP0 (first byte of ESP)
		movb $SEL_SDATA,TSS_SS0(%di)	# Set SS0
		movb $_ESP1H,TSS_ESP1+1(%di)	# Set ESP1
		movb $_TSSIO,TSS_MAP(%di)		# Set I/O bit map base
ifdef(`PAGING',`
#
# Create page directory.
#
		xor %edx,%edx			# Page
		mov $PAG_SIZ>>0x8,%dh		#  size
		xor %eax,%eax			# Zero
		mov $MEM_DIR,%di		# Page directory
		mov $PAG_CNT>>0xa,%cl		# Entries
		mov $MEM_TBL|0x7,%ax	 	# First entry
init.5: 	stosl				# Write entry
		add %dx,%ax			# To next
		loop init.5			# Till done
#
# Create page tables.
#
		mov $MEM_TBL,%di		# Page table
		mov $PAG_CNT>>0x8,%ch		# Entries
		xor %ax,%ax			# Start address
init.6: 	mov $0x7,%al			# Set U:W:P flags
		cmp btx_hdr+0x8,%cx	 	# Standard user page?
		jb init.7			# Yes
		cmp $PAG_CNT-MEM_BTX>>0xc,%cx	# BTX memory?
		jae init.7			# No or first page
		and $~0x2,%al			# Clear W flag
		cmp $PAG_CNT-MEM_USR>>0xc,%cx	# User page zero?
		jne init.7			# No
		testb $0x80,btx_hdr+0x7		# Unmap it?
		jz init.7			# No
		and $~0x1,%al			# Clear P flag
init.7: 	stosl				# Set entry
		add %edx,%eax			# Next address
		loop init.6			# Till done
')
#
# Bring up the system.
#
		mov $0x2820,%bx			# Set protected mode
		callw setpic			#  IRQ offsets
		lidt idtdesc	 		# Set IDT
ifdef(`PAGING',`
		xor %eax,%eax			# Set base
		mov $MEM_DIR>>0x8,%ah		#  of page
		mov %eax,%cr3			#  directory
')
		lgdt gdtdesc	 		# Set GDT
		mov %cr0,%eax			# Switch to protected
ifdef(`PAGING',`
		or $0x80000001,%eax             #  mode and enable paging
',`
		or $0x01,%eax			#  mode
')
		mov %eax,%cr0			#  
		ljmp $SEL_SCODE,$init.8		# To 32-bit code
		.code32
init.8:
		xorl %ecx,%ecx			# Zero
		movb $SEL_SDATA,%cl		# To 32-bit
		movw %cx,%ss			#  stack
#
# Launch user task.
#
		movb $SEL_TSS,%cl		# Set task
		ltr %cx				#  register
		movl $MEM_USR,%edx		# User base address
		movzwl %ss:BDA_MEM,%eax 	# Get free memory
		shll $0xa,%eax			# To bytes
		subl $0x1000,%eax		# Less arg space
		subl %edx,%eax			# Less base
		movb $SEL_UDATA,%cl		# User data selector
		pushl %ecx			# Set SS
		pushl %eax			# Set ESP
		push $0x202			# Set flags (IF set)
		push $SEL_UCODE			# Set CS
		pushl btx_hdr+0xc		# Set EIP
		pushl %ecx			# Set GS
		pushl %ecx			# Set FS
		pushl %ecx			# Set DS
		pushl %ecx			# Set ES
		pushl %edx			# Set EAX
		movb $0x7,%cl			# Set remaining
init.9:
		push $0x0			#  general
		loop init.9			#  registers
ifdef(`BTX_SERIAL',`
		call sio_init			# setup the serial console
')
		popa				#  and initialize
		popl %es			# Initialize
		popl %ds			#  user
		popl %fs			#  segment
		popl %gs			#  registers
		iret				# To user mode

int
main(void)
{
    int autoboot;
    ino_t ino;

    dmadat = (void *)(roundup2(__base + (int32_t)&_end, 0x10000) - __base);
    v86.ctl = V86_FLAGS;	// v86.ctl = 0x40000
    dsk.drive = *(uint8_t *)PTOV(ARGS);
    dsk.type = dsk.drive & DRV_HARD ? TYPE_AD : TYPE_FD;
    dsk.unit = dsk.drive & DRV_MASK;
    dsk.slice = *(uint8_t *)PTOV(ARGS + 1) + 1;
    bootinfo.bi_version = BOOTINFO_VERSION;		// bi_version = 0x1
    bootinfo.bi_size = sizeof(bootinfo);
    bootinfo.bi_basemem = 0;	/* XXX will be filled by loader or kernel */
    bootinfo.bi_extmem = memsize();		// Use V86 Mode to execute BIOS INT 0x88
										// to obtain extended memory size
    bootinfo.bi_memsizes_valid++;

    /* Process configuration file */

    autoboot = 1;

	/* Check for /boot.config file */
    if ((ino = lookup(PATH_CONFIG)))	// ino = lookup("/boot.config")
	fsread(ino, cmd, sizeof(cmd));		// cmd is a static global variable

    if (*cmd) {
	printf("%s: %s", PATH_CONFIG, cmd);	// echoes contents of boot.config to console
										//  Ex. Default: 0:ad(0,a)/boot/loader (used by autoboot)
										// Ex. 1:ad(1,a)/boot/loader (obtained from boot.config)

	/* parse() sets disk info for /boot/loader and sets kname */
	if (parse())
	    autoboot = 0;	/* set to 0 for invalid config file */
	/* Do not process this command twice */
	*cmd = 0;
    }

    /*
     * Try to exec stage 3 boot loader. If interrupted by a keypress,
     * or in case of failure, try to load a kernel directly instead.
     */

    if (autoboot && !*kname) {
		memcpy(kname, PATH_BOOT3, sizeof(PATH_BOOT3));	/* kname = "/boot/loader" */

	/*
    ** If the user doesn't press a key within three seconds, load the stage 3
	** boot loader.
	*/
	if (!keyhit(3*SECOND)) {
	    load();
	    memcpy(kname, PATH_KERNEL, sizeof(PATH_KERNEL));
	}
    }

    /* Present the user with the boot2 prompt. */

    for (;;) {
	printf("\nFreeBSD/i386 boot\n"
	       "Default: %u:%s(%u,%c)%s\n"
	       "boot: ",
	       dsk.drive & DRV_MASK, dev_nm[dsk.type], dsk.unit,
	       'a' + dsk.part, kname);
	if (ioctrl & IO_SERIAL)
	    sio_flush();
	if (!autoboot || keyhit(5*SECOND))
	    getstr();
	else
	    putchar('\n');
	autoboot = 0;
	if (parse())
	    putchar('\a');
	else
	    load();
    }
}

static void
load(void)
{
    union {
	struct exec ex;
	Elf32_Ehdr eh;
    } hdr;					// Union for parsing kernel binary's header
    Elf32_Phdr ep[2];
    Elf32_Shdr es[2];
    caddr_t p;
    ino_t ino;
    uint32_t addr, x;
    int fmt, i, j;

    if (!(ino = lookup(kname))) {	// Invalid path or not reg file
	if (!ls)
	    printf("No %s\n", kname);
	return;
    }

    if (xfsread(ino, &hdr, sizeof(hdr)))	// Invalid header format
		return;
    if (N_GETMAGIC(hdr.ex) == ZMAGIC)	// ZMAGIC = 0413, demand load format
		fmt = 0;
    else if (IS_ELF(hdr.eh))	// { 0x7f, 'E', 'L', 'F' }
		fmt = 1;
    else
	{
		printf("Invalid %s\n", "format");
		return;
    }

	/*
	** Demand Load Format
	*/
    if (fmt == 0)
	{
		addr = hdr.ex.a_entry & 0xffffff;
		p = PTOV(addr);							// p = vaddr of kernel entry point
		fs_off = PAGE_SIZE;						// fs_off = 0x1000
		if (xfsread(ino, p, hdr.ex.a_text))		// Write .text in memory
	    	return;

		p += roundup2(hdr.ex.a_text, PAGE_SIZE);	// Increment p by sizeof(.text)

		if (xfsread(ino, p, hdr.ex.a_data))			// Write .data in memory
	    	return;

		p += hdr.ex.a_data + roundup2(hdr.ex.a_bss, PAGE_SIZE);	// Increment p by sizeof(.data)

		bootinfo.bi_symtab = VTOP(p);							// Record location of symbol table
		memcpy(p, &hdr.ex.a_syms, sizeof(hdr.ex.a_syms));		// Write size of the symbol table in memory
		p += sizeof(hdr.ex.a_syms);								// Increment p by 4 bytes (a_syms is a long)

		if (hdr.ex.a_syms)	// If the symbol table has size > 0
		{
	    	if (xfsread(ino, p, hdr.ex.a_syms))		// Write the symbol table to memory
				return;
	    	p += hdr.ex.a_syms;						// Increment p by the size of the memory table
	    	if (xfsread(ino, p, sizeof(int)))
				return;
	    	x = *(uint32_t *)p;
	    	p += sizeof(int);
	    	x -= sizeof(int);
	    	if (xfsread(ino, p, x))
				return;
	    	p += x;
		}
    }
	else	// ELF Header 
	{
		fs_off = hdr.eh.e_phoff;	// Store program header table file offset

		/*
		** Store the first two loadable program header table entries
		** into ep array.
		*/
		for (j = i = 0; i < hdr.eh.e_phnum && j < 2; i++)
		{
	    	if (xfsread(ino, ep + j, sizeof(ep[0])))
				return;
	 	   if (ep[j].p_type == PT_LOAD)		// program header loadable?
			j++;
		}

		for (i = 0; i < 2; i++)		// Write two program headers into memory
		{
	    	p = PTOV(ep[i].p_paddr & 0xffffff);		// p = vaddr of the prog header
	    	fs_off = ep[i].p_offset;				// Set file offset equal to
													// the program header offset
	    	if (xfsread(ino, p, ep[i].p_filesz))
				return;
		}
		p += roundup2(ep[1].p_memsz, PAGE_SIZE);	// Increment p by size of last header

		bootinfo.bi_symtab = VTOP(p);	// Store symbol table vaddr
		if (hdr.eh.e_shnum == hdr.eh.e_shstrndx + 3)	// Something obscure
		{
			/*
			** Set file offset equal to the section header that contains the
			** section names
			*/
	    	fs_off = hdr.eh.e_shoff + sizeof(es[0]) * (hdr.eh.e_shstrndx + 1);

			// Read two section header table entries into es
	    	if (xfsread(ino, &es, sizeof(es)))
				return;
	    	for (i = 0; i < 2; i++)	// Read two section headers into memory
			{
				// Copy section header's size to memory
				memcpy(p, &es[i].sh_size, sizeof(es[i].sh_size));
				p += sizeof(es[i].sh_size);
				fs_off = es[i].sh_offset;
				if (xfsread(ino, p, es[i].sh_size)) 
		    		return;
				p += es[i].sh_size;
	    	}
		}
		addr = hdr.eh.e_entry & 0xffffff;	// Set address to loader entry point
    }
    bootinfo.bi_esymtab = VTOP(p);
    bootinfo.bi_kernelname = VTOP(kname);
    bootinfo.bi_bios_dev = dsk.drive;

	// Jump to /boot/loader's entry point
    __exec((caddr_t)addr, RB_BOOTINFO | (opts & RBX_MASK),
	   MAKEBOOTDEV(dev_maj[dsk.type], 0, dsk.slice, dsk.unit, dsk.part),
	   0, 0, 0, VTOP(&bootinfo));
}

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

int
main(void)
{
    int		i;

    /* Pick up arguments */
    kargs = (void *)__args;		// __args is extern var defined in btxv86.h
    initial_howto = kargs->howto;
    initial_bootdev = kargs->bootdev;
    initial_bootinfo = kargs->bootinfo ? (struct bootinfo *)PTOV(kargs->bootinfo) : NULL;

    /* 
     * Initialise the heap as early as possible.  Once this is done, malloc() is usable.
     */
    bios_getmem();

    setheap((void *)end, (void *)bios_basemem);	// Cannot locate this function

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

    /*
     * Initialise the block cache
     */
    bcache_init(32, 512);	/* 16k cache XXX tune this */

    /*
     * Special handling for PXE and CD booting.
     */
    if (kargs->bootinfo == NULL) {
	/*
	 * We only want the PXE disk to try to init itself in the below
	 * walk through devsw if we actually booted off of PXE.
	 */
	if (kargs->bootflags & KARGS_FLAGS_PXE)
	    pxe_enable(kargs->pxeinfo ? PTOV(kargs->pxeinfo) : NULL);
	else if (kargs->bootflags & KARGS_FLAGS_CD)
	    bc_add(initial_bootdev);
    }

    /*
     * March through the device switch probing for things.
     */
    for (i = 0; devsw[i] != NULL; i++)
	if (devsw[i]->dv_init != NULL)
	    (devsw[i]->dv_init)();
    printf("BIOS %dkB/%dkB available memory\n", bios_basemem / 1024, bios_extmem / 1024);
    if (initial_bootinfo != NULL) {
	initial_bootinfo->bi_basemem = bios_basemem / 1024;
	initial_bootinfo->bi_extmem = bios_extmem / 1024;
    }

    /* detect ACPI for future reference */
    biosacpi_detect();

    printf("\n");
    printf("%s, Revision %s\n", bootprog_name, bootprog_rev);
    printf("(%s, %s)\n", bootprog_maker, bootprog_date);

    extract_currdev();				/* set $currdev and $loaddev */
    setenv("LINES", "24", 1);		/* optional */
    
    bios_getsmap();

    archsw.arch_autoload = i386_autoload;
    archsw.arch_getdev = i386_getdev;
    archsw.arch_copyin = i386_copyin;
    archsw.arch_copyout = i386_copyout;
    archsw.arch_readin = i386_readin;
    archsw.arch_isainb = isa_inb;
    archsw.arch_isaoutb = isa_outb;

    interact();			/* doesn't return */

    /* if we ever get here, it is an error */
    return (1);
}

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
	pushl	$PSL_KERNEL
	popfl

/*
 * Don't trust what the BIOS gives for %fs and %gs.  Trust the bootstrap
 * to set %cs, %ds, %es and %ss.
 */
	mov	%ds, %ax
	mov	%ax, %fs
	mov	%ax, %gs

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
 * Note 2: $R() is just a macro for vaddr -> paddr by subtracting kernbase
 */
	movl	$R(end),%ecx
	movl	$R(edata),%edi
	subl	%edi,%ecx
	xorl	%eax,%eax
	cld
	rep
	stosb

	call	recover_bootinfo

/* Get onto a stack that we can trust. */
/*
 * XXX this step is delayed in case recover_bootinfo needs to return via
 * the old stack, but it need not be, since recover_bootinfo actually
 * returns via the old frame.
 */
	movl	$R(tmpstk),%esp	/* tmpstk at base of data segment */

	call	identify_cpu

/*
 * 1. Round up the end of the kernel image to the nearest 4MB boundary
 * 2. Allocate, set p/va's, and init to 0:
 *       30 pgs for the KPTphys
 *        1 pg for IdlePTD
 *        1 pg for p0's u_area
 *        2 pgs for p0's kernel stk
 *        1 pg for vm86/bios stack
 *        3 pgs for pgtable + ext + IOPAGES
 * 3. Set first pg rw for bios32 calls
 * 4. Set pg 1 - etext to read-only
 * 5. Turn on Page Size Extension (PSE) and Global Page (PGE)
 * 6. Fill rw pgs of the kernel image starting from btext to end 
 * 7. Fill rw pages for IdlePTD, p0, isa hole, and vm86
 * 8. Map pg 0 and ISA hole into vm86 page table
 * 9. Finally, set PDEs for all the KPTs, including a recursively mapped
 *    PDE for the PD itself.
 */
	call	create_pagetables

/*
 * If the CPU has support for VME, turn it on.
 */ 
	testl	$CPUID_VME, R(cpu_feature)
	jz	1f
	movl	%cr4, %eax
	orl	$CR4_VME, %eax
	movl	%eax, %cr4
1:

/* Now enable paging */
	movl	R(IdlePTD), %eax
	movl	%eax,%cr3		/* load ptd addr into mmu */
	movl	%cr0,%eax		/* get control word */
	orl	$CR0_PE|CR0_PG,%eax	/* enable paging */
	movl	%eax,%cr0		/* and let's page NOW! */

	pushl	$begin			/* jump to high virtualized address */
	ret

/* now running relocated at KERNBASE where the system is linked to run */
begin:
	/* set up bootstrap stack */
	movl	proc0kstack,%eax	/* location of in-kernel stack */
			/* bootstrap stack end location */
	leal	(KSTACK_PAGES*PAGE_SIZE-PCB_SIZE)(%eax),%esp

	xorl	%ebp,%ebp		/* mark end of frames */

	movl	IdlePTD,%esi
	movl	%esi,(KSTACK_PAGES*PAGE_SIZE-PCB_SIZE+PCB_CR3)(%eax)

	pushl	physfree		/* value of first for init386(first) */
	call	init386			/* wire 386 chip for unix operation */

	/*
	 * Clean up the stack in a way that db_numargs() understands, so
	 * that backtraces in ddb don't underrun the stack.  Traps for
	 * inaccessible memory are more fatal than usual this early.
	 */
	addl	$4,%esp

	call	mi_startup		/* autoconfiguration, mountroot etc */

void
init386(first)
	int first;
{
	struct gate_descriptor *gdp;
	int gsel_tss, metadata_missing, off, x;
	struct pcpu *pc;

	/* proc0 and thread0 are global structs def in init_main.c */
	proc0.p_uarea = proc0uarea;
	thread0.td_kstack = proc0kstack;
	thread0.td_pcb = (struct pcb *)	/* pcb at top of kstack, not bot */
	   (thread0.td_kstack + KSTACK_PAGES * PAGE_SIZE) - 1;
	atdevbase = ISA_HOLE_START + KERNBASE;	/* a0000h + KERNBASE */

	/*
 	 * This may be done better later if it gets more high level
 	 * components in it. If so just link td->td_proc here.
	 */
	proc_linkup(&proc0, &ksegrp0, &kse0, &thread0);

	metadata_missing = 0;
	if (bootinfo.bi_modulep) {
		preload_metadata = (caddr_t)bootinfo.bi_modulep + KERNBASE;
		preload_bootstrap_relocate(KERNBASE);
	} else {
		metadata_missing = 1;
	}
	if (envmode == 1)
		kern_envp = static_env;
	else if (bootinfo.bi_envp)
		kern_envp = (caddr_t)bootinfo.bi_envp + KERNBASE;

	/* Init basic tunables, hz etc
	 *
	 * 1. Set tick = 10,000
	 * 2. ???
	 */
	init_param1();

	/*
	 * make gdt memory segments, the code segment goes up to end of the
	 * page with etext in it, the data segment goes to the end of
	 * the address space.
	 *
	 * NOTE: This uses the same algorithm for creating the GDT as 386BSD.
	 */
	/*
	 * XXX text protection is temporarily (?) disabled.  The limit was
	 * i386_btop(round_page(etext)) - 1.
	 */
	gdt_segs[GCODE_SEL].ssd_limit = atop(0 - 1);	/* limit is end of addr space */
	gdt_segs[GDATA_SEL].ssd_limit = atop(0 - 1);	/* ditto */

	pc = &__pcpu;	/* static per-cpu structure def in machdep.c */

	/* __pcpu's dedicated gdt segment */
	gdt_segs[GPRIV_SEL].ssd_limit =
		atop(sizeof(struct pcpu) - 1);
	gdt_segs[GPRIV_SEL].ssd_base = (int) pc;

	/* in SMP, CPUs share a tss */
	gdt_segs[GPROC0_SEL].ssd_base = (int) &pc->pc_common_tss;

	for (x = 0; x < NGDT; x++)
		ssdtosd(&gdt_segs[x], &gdt[x].sd);

	r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
	r_gdt.rd_base =  (int) gdt;
	lgdt(&r_gdt);

	/* 1. bzero pc
	 * 2. Check if cpuid is valid. If it is invalid, write err msg to prompt
	 * 3. Set cpuid and cpuid mask in pc
	 * 4. Add pc to cpu list
	 * 5. MD initialization
	 */
	pcpu_init(pc, 0, sizeof(struct pcpu));

	/* set prvspace and curthread using macros defined in pcpu.h */
	PCPU_SET(prvspace, pc);
	PCPU_SET(curthread, &thread0);

	/*
	 * Initialize mutexes.
	 *
	 * icu_lock: in order to allow an interrupt to occur in a critical
	 * 	     section, to set pcpu->ipending (etc...) properly, we
	 *	     must be able to get the icu lock, so it can't be
	 *	     under witness.
	 *
	 *   1. Initialize thread0's lock (td_contested)
	 *   2. Initialize turnstile_chains array
	 *   3. Initialize static mutex td_contested_lock
	 *   4. Set thread0's mutex to NULL
	 *   5. Initialize 3 mutexes (Giant, sched lock, process lock) and lock Giant
	 */
	mutex_init();

	/* The NULL arg means that these locks will have their names as their type */
	mtx_init(&clock_lock, "clk", NULL, MTX_SPIN);
	mtx_init(&icu_lock, "icu", NULL, MTX_SPIN | MTX_NOWITNESS);

	/* make ldt memory segments */
	/*
	 * XXX - VM_MAXUSER_ADDRESS is an end address, not a max.  And it
	 * should be spelled ...MAX_USER...
	 */
	ldt_segs[LUCODE_SEL].ssd_limit = atop(VM_MAXUSER_ADDRESS - 1);
	ldt_segs[LUDATA_SEL].ssd_limit = atop(VM_MAXUSER_ADDRESS - 1);
	for (x = 0; x < sizeof ldt_segs / sizeof ldt_segs[0]; x++)
		ssdtosd(&ldt_segs[x], &ldt[x].sd);

	/* remember that GSEL(sel, prio) = ((sel << 3) | prio ) */
	_default_ldt = GSEL(GLDT_SEL, SEL_KPL);
	lldt(_default_ldt);
	PCPU_SET(currentldt, _default_ldt);

	/* exceptions */
	/*
	 * First, initialize all 256 int vectors to have kernel priority
	 * and point to the rsvd label.
	 */
	for (x = 0; x < NIDT; x++)
		setidt(x, &IDTVEC(rsvd), SDT_SYS386TGT, SEL_KPL,
		    GSEL(GCODE_SEL, SEL_KPL));

	/*
	 * Set Intel exceptions and syscall int0x80
	 */
	setidt(IDT_DE, &IDTVEC(div),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_DB, &IDTVEC(dbg),  SDT_SYS386IGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_NMI, &IDTVEC(nmi),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
 	setidt(IDT_BP, &IDTVEC(bpt),  SDT_SYS386IGT, SEL_UPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_OF, &IDTVEC(ofl),  SDT_SYS386TGT, SEL_UPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_BR, &IDTVEC(bnd),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_UD, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_NM, &IDTVEC(dna),  SDT_SYS386TGT, SEL_KPL
	    , GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_DF, 0,  SDT_SYSTASKGT, SEL_KPL, GSEL(GPANIC_SEL, SEL_KPL));
	setidt(IDT_FPUGP, &IDTVEC(fpusegm),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_TS, &IDTVEC(tss),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_NP, &IDTVEC(missing),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_SS, &IDTVEC(stk),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_GP, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_PF, &IDTVEC(page),  SDT_SYS386IGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_MF, &IDTVEC(fpu),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_AC, &IDTVEC(align), SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_MC, &IDTVEC(mchk),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_XF, &IDTVEC(xmm), SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
 	setidt(IDT_SYSCALL, &IDTVEC(int0x80_syscall), SDT_SYS386TGT, SEL_UPL,
	    GSEL(GCODE_SEL, SEL_KPL));

	r_idt.rd_limit = sizeof(idt0) - 1;
	r_idt.rd_base = (int) idt;
	lidt(&r_idt);

	/*
	 * Initialize the console before we print anything out.
	 * 
	 * Checks for the highest priority console and initializes it. If RB_MULTIPLE
	 * is set, we add and initialize each console in the linker set.
	 */
	cninit();

	if (metadata_missing)
		printf("WARNING: loader(8) metadata is missing!\n");

/* 8259 PIC Controller init. The 'at' presumably refers to IBM PC AT */
#ifdef DEV_ISA
	atpic_startup();
#endif

/* kernel debugger */
#ifdef DDB
	kdb_init();
	if (boothowto & RB_KDB)
		Debugger("Boot flags requested debugger");
#endif

	finishidentcpu();	/* Final stage of CPU initialization */
	setidt(IDT_UD, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setidt(IDT_GP, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	initializecpu();	/* Initialize CPU registers */

	/* make an initial tss so cpu can get interrupt stack on syscall! */
	/* Note: -16 is so we can grow the trapframe if we came from vm86 */
	PCPU_SET(common_tss.tss_esp0, thread0.td_kstack +
	    KSTACK_PAGES * PAGE_SIZE - sizeof(struct pcb) - 16);
	PCPU_SET(common_tss.tss_ss0, GSEL(GDATA_SEL, SEL_KPL));
	gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	private_tss = 0; 

	/* Recall that gdt is an array of unions; sd is the segment_descriptor */
	PCPU_SET(tss_gdt, &gdt[GPROC0_SEL].sd);
	PCPU_SET(common_tssd, *PCPU_GET(tss_gdt));
	PCPU_SET(common_tss.tss_ioopt, (sizeof (struct i386tss)) << 16);
	ltr(gsel_tss);

	dblfault_tss.tss_esp = dblfault_tss.tss_esp0 = dblfault_tss.tss_esp1 =
	    dblfault_tss.tss_esp2 = (int)&dblfault_stack[sizeof(dblfault_stack)];
	dblfault_tss.tss_ss = dblfault_tss.tss_ss0 = dblfault_tss.tss_ss1 =
	    dblfault_tss.tss_ss2 = GSEL(GDATA_SEL, SEL_KPL);
	dblfault_tss.tss_cr3 = (int)IdlePTD;
	dblfault_tss.tss_eip = (int)dblfault_handler;
	dblfault_tss.tss_eflags = PSL_KERNEL;
	dblfault_tss.tss_ds = dblfault_tss.tss_es =
	    dblfault_tss.tss_gs = GSEL(GDATA_SEL, SEL_KPL);
	dblfault_tss.tss_fs = GSEL(GPRIV_SEL, SEL_KPL);
	dblfault_tss.tss_cs = GSEL(GCODE_SEL, SEL_KPL);
	dblfault_tss.tss_ldt = GSEL(GLDT_SEL, SEL_KPL);

	vm86_initialize();

	/*
	 * 1. Use int 0x12 to obtain the size of base mem in KiB. If the size is
	 *     < 640KiB, we map the difference as rw pages into vm86's pg table.
	 * 2. Use int 0x15 E820 to fill the physmap with a list of free mem regions.
	 *    The SMAP structures the call uses are two quadwords: the first is the
	 *    base, the second is the length.
	 * 3. If int 0x15 E820 failed, we try to use 0x15 E801. And if that fails
	 *    we use the RTC value for ext mem.
	 * 4. Obtain the last physical page number of memory called Maxmem.
	 * 5. Call pmap_bootstrap()
	 * 6. Test each page of free memory contained in the physmap and make sure
	 *    we have enough space for the message buffer at the end of core.
	 */
	getmemsize(first);
	init_param2(physmem);

	/* now running on new page tables, configured,and u/iom is accessible */

	/* Map the message buffer. */
	for (off = 0; off < round_page(MSGBUF_SIZE); off += PAGE_SIZE)
		pmap_kenter((vm_offset_t)msgbufp + off, avail_end + off);

	/*
	 * Checks to see if the contents of the old msgbuf can be recovered by
	 * checking a magic number at the end of the buffer. If the magic number
	 * is valid and the checksum is also valid, we keep the old contents.
	 * Otherwise, we clear the old contents.
	 */
	msgbufinit(msgbufp, MSGBUF_SIZE);

	/* make a call gate to reenter kernel with */
	gdp = &ldt[LSYS5CALLS_SEL].gd;	/* gd = gate descr */

	x = (int) &IDTVEC(lcall_syscall);
	gdp->gd_looffset = x;
	gdp->gd_selector = GSEL(GCODE_SEL,SEL_KPL);
	gdp->gd_stkcpy = 1;
	gdp->gd_type = SDT_SYS386CGT;
	gdp->gd_dpl = SEL_UPL;
	gdp->gd_p = 1;
	gdp->gd_hioffset = x >> 16;

	/* XXX does this work? */
	ldt[LBSDICALLS_SEL] = ldt[LSYS5CALLS_SEL];
	ldt[LSOL26CALLS_SEL] = ldt[LSYS5CALLS_SEL];

	/* transfer to user mode */

	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc 0's pcb */
	thread0.td_pcb->pcb_flags = 0; /* XXXKSE */
	thread0.td_pcb->pcb_cr3 = (int)IdlePTD;
	thread0.td_pcb->pcb_ext = 0;
	thread0.td_frame = &proc0_tf;
}

/*
 * System startup; initialize the world, create process 0, mount root
 * filesystem, and fork to create init and pagedaemon.  Most of the
 * hard work is done in the lower-level initialization routines including
 * startup(), which does memory initialization and autoconfiguration.
 *
 * This allows simple addition of new kernel subsystems that require
 * boot time initialization.  It also allows substitution of subsystem
 * (for instance, a scheduler, kernel profiler, or VM system) by object
 * module.  Finally, it allows for optional "kernel threads".
 */
void
mi_startup(void)
{

	register struct sysinit **sipp;		/* system initialization*/
	register struct sysinit **xipp;		/* interior loop of sort*/
	register struct sysinit *save;		/* bubble*/

	if (sysinit == NULL) {
		sysinit = SET_BEGIN(sysinit_set);
		sysinit_end = SET_LIMIT(sysinit_set);
	}

restart:
	/*
	 * Perform a bubble sort of the system initialization objects by
	 * their subsystem (primary key) and order (secondary key).
	 */
	for (sipp = sysinit; sipp < sysinit_end; sipp++) {
		for (xipp = sipp + 1; xipp < sysinit_end; xipp++) {
			if ((*sipp)->subsystem < (*xipp)->subsystem ||
			     ((*sipp)->subsystem == (*xipp)->subsystem &&
			      (*sipp)->order <= (*xipp)->order))
				continue;	/* skip*/
			save = *sipp;
			*sipp = *xipp;
			*xipp = save;
		}
	}

	/*
	 * Traverse the (now) ordered list of system initialization tasks.
	 * Perform each task, and continue on to the next task.
	 *
	 * The last item on the list is expected to be the scheduler,
	 * which will not return.
	 */
	for (sipp = sysinit; sipp < sysinit_end; sipp++) {

		if ((*sipp)->subsystem == SI_SUB_DUMMY)
			continue;	/* skip dummy task(s)*/

		if ((*sipp)->subsystem == SI_SUB_DONE)
			continue;

		/* Call function */
		(*((*sipp)->func))((*sipp)->udata);

		/* Check off the one we're just done */
		(*sipp)->subsystem = SI_SUB_DONE;

		/* Check if we've installed more sysinit items via KLD */
		if (newsysinit != NULL) {
			if (sysinit != SET_BEGIN(sysinit_set))
				free(sysinit, M_TEMP);
			sysinit = newsysinit;
			sysinit_end = newsysinit_end;
			newsysinit = NULL;
			newsysinit_end = NULL;
			goto restart;
		}
	}

	panic("Shouldn't get here!");
	/* NOTREACHED*/
}
```

## Allocating and Filling KPT Entries

IdlePTD is the page directory we set to CR4 and it follows the physically wired
kernel page tables in memory.

```c
#define R(foo) ((foo)-KERNBASE)

/*
 * Allocate foo pages of memory and initialize them to zero.
 *
 * Arguments:
 *   %esi/%edi = free pointer
 *   %eax = size of allocation in bytes, then 0 for stosb
 */
#define ALLOCPAGES(foo) \
	movl	R(physfree), %esi ; \
	movl	$((foo)*PAGE_SIZE), %eax ; \
	addl	%esi, %eax ; \
	movl	%eax, R(physfree) ; \
	movl	%esi, %edi ; \
	movl	$((foo)*PAGE_SIZE),%ecx ; \
	xorl	%eax,%eax ; \
	cld ; \
	rep ; \
	stosb

/*
 * fillkpt
 *	eax = page frame address
 *	ebx = index into page table
 *	ecx = how many pages to map
 * 	base = base address of page dir/table
 *	prot = protection bits
 */
#define	fillkpt(base, prot)		  \
	shll	$PTESHIFT,%ebx		; \	/* shift idx 2 bits */
	addl	base,%ebx		; \
	orl	$PG_V,%eax		; \			/* set valid */
	orl	prot,%eax		; \			/* set prot */
1:	movl	%eax,(%ebx)		; \		/* write entry */
	addl	$PAGE_SIZE,%eax		;	/* increment physical address */ \
	addl	$PTESIZE,%ebx		;	/* next pte */ \
	loop	1b

/*
 * fillkptphys(prot)
 *	eax = physical address
 *	ecx = how many pages to map
 *	prot = protection bits
 */
#define	fillkptphys(prot)		  \
	movl	%eax, %ebx		; \
	shrl	$PAGE_SHIFT, %ebx	; \	/* convert to pg frame nb */
	fillkpt(R(KPTphys), prot)
```

## SYSINIT Initialization Order

```c
/*
 * Enumerated types for known system startup interfaces.
 *
 * Startup occurs in ascending numeric order; the list entries are
 * sorted prior to attempting startup to guarantee order.  Items
 * of the same level are arbitrated for order based on the 'order'
 * element.
 *
 * These numbers are arbitrary and are chosen ONLY for ordering; the
 * enumeration values are explicit rather than implicit to provide
 * for binary compatibility with inserted elements.
 *
 * The SI_SUB_RUN_SCHEDULER value must have the highest lexical value.
 *
 * The SI_SUB_CONSOLE and SI_SUB_SWAP values represent values used by
 * the BSD 4.4Lite but not by FreeBSD; they are maintained in dependent
 * order to support porting.
 *
 * The SI_SUB_PROTO_BEGIN and SI_SUB_PROTO_END bracket a range of
 * initializations to take place at splimp().  This is a historical
 * wart that should be removed -- probably running everything at
 * splimp() until the first init that doesn't want it is the correct
 * fix.  They are currently present to ensure historical behavior.
 */
enum sysinit_sub_id {
	SI_SUB_DUMMY		= 0x0000000,	/* not executed; for linker*/
	SI_SUB_DONE		= 0x0000001,	/* processed*/
	SI_SUB_TUNABLES		= 0x0700000,	/* establish tunable values */
	SI_SUB_CONSOLE		= 0x0800000,	/* console*/
	SI_SUB_COPYRIGHT	= 0x0800001,	/* first use of console*/
	SI_SUB_MTX_POOL_STATIC	= 0x0900000,	/* static mutex pool */
	SI_SUB_LOCKMGR		= 0x0980000,	/* lockmgr locks */
	SI_SUB_VM		= 0x1000000,	/* virtual memory system init*/
	SI_SUB_KMEM		= 0x1800000,	/* kernel memory*/
	SI_SUB_KVM_RSRC		= 0x1A00000,	/* kvm operational limits*/
	SI_SUB_WITNESS		= 0x1A80000,	/* witness initialization */
	SI_SUB_MTX_POOL_DYNAMIC	= 0x1AC0000,	/* dynamic mutex pool */
	SI_SUB_LOCK		= 0x1B00000,	/* various locks */
	SI_SUB_EVENTHANDLER	= 0x1C00000,	/* eventhandler init */
	SI_SUB_KLD		= 0x2000000,	/* KLD and module setup */
	SI_SUB_CPU		= 0x2100000,	/* CPU resource(s)*/
	SI_SUB_MAC		= 0x2180000,	/* TrustedBSD MAC subsystem */
	SI_SUB_MAC_POLICY	= 0x21C0000,	/* TrustedBSD MAC policies */
	SI_SUB_MAC_LATE		= 0x21D0000,	/* TrustedBSD MAC subsystem */
	SI_SUB_INTRINSIC	= 0x2200000,	/* proc 0*/
	SI_SUB_VM_CONF		= 0x2300000,	/* config VM, set limits*/
	SI_SUB_RUN_QUEUE	= 0x2400000,	/* set up run queue*/
	SI_SUB_KTRACE		= 0x2480000,	/* ktrace */
	SI_SUB_CREATE_INIT	= 0x2500000,	/* create init process*/
	SI_SUB_SCHED_IDLE	= 0x2600000,	/* required idle procs */
	SI_SUB_MBUF		= 0x2700000,	/* mbuf subsystem */
	SI_SUB_INTR		= 0x2800000,	/* interrupt threads */
	SI_SUB_SOFTINTR		= 0x2800001,	/* start soft interrupt thread */
	SI_SUB_DEVFS		= 0x2F00000,	/* devfs ready for devices */
	SI_SUB_INIT_IF		= 0x3000000,	/* prep for net interfaces */
	SI_SUB_DRIVERS		= 0x3100000,	/* Let Drivers initialize */
	SI_SUB_CONFIGURE	= 0x3800000,	/* Configure devices */
	SI_SUB_VFS		= 0x4000000,	/* virtual filesystem*/
	SI_SUB_CLOCKS		= 0x4800000,	/* real time and stat clocks*/
	SI_SUB_CLIST		= 0x5800000,	/* clists*/
	SI_SUB_SYSV_SHM		= 0x6400000,	/* System V shared memory*/
	SI_SUB_SYSV_SEM		= 0x6800000,	/* System V semaphores*/
	SI_SUB_SYSV_MSG		= 0x6C00000,	/* System V message queues*/
	SI_SUB_P1003_1B		= 0x6E00000,	/* P1003.1B realtime */
	SI_SUB_PSEUDO		= 0x7000000,	/* pseudo devices*/
	SI_SUB_EXEC		= 0x7400000,	/* execve() handlers */
	SI_SUB_PROTO_BEGIN	= 0x8000000,	/* XXX: set splimp (kludge)*/
	SI_SUB_PROTO_IF		= 0x8400000,	/* interfaces*/
	SI_SUB_PROTO_DOMAIN	= 0x8800000,	/* domains (address families?)*/
	SI_SUB_PROTO_IFATTACHDOMAIN	= 0x8800001,	/* domain dependent data init*/
	SI_SUB_PROTO_END	= 0x8ffffff,	/* XXX: set splx (kludge)*/
	SI_SUB_KPROF		= 0x9000000,	/* kernel profiling*/
	SI_SUB_KICK_SCHEDULER	= 0xa000000,	/* start the timeout events*/
	SI_SUB_INT_CONFIG_HOOKS	= 0xa800000,	/* Interrupts enabled config */
	SI_SUB_ROOT_CONF	= 0xb000000,	/* Find root devices */
	SI_SUB_DUMP_CONF	= 0xb200000,	/* Find dump devices */
	SI_SUB_RAID		= 0xb380000,	/* Configure RAIDframe or Vinum */
	SI_SUB_MOUNT_ROOT	= 0xb400000,	/* root mount*/
	SI_SUB_SWAP		= 0xc000000,	/* swap */
	SI_SUB_INTRINSIC_POST	= 0xd000000,	/* proc 0 cleanup*/
	SI_SUB_KTHREAD_INIT	= 0xe000000,	/* init process*/
	SI_SUB_KTHREAD_PAGE	= 0xe400000,	/* pageout daemon*/
	SI_SUB_KTHREAD_VM	= 0xe800000,	/* vm daemon*/
	SI_SUB_KTHREAD_BUF	= 0xea00000,	/* buffer daemon*/
	SI_SUB_KTHREAD_UPDATE	= 0xec00000,	/* update daemon*/
	SI_SUB_KTHREAD_IDLE	= 0xee00000,	/* idle procs*/
	SI_SUB_SMP		= 0xf000000,	/* start the APs*/
	SI_SUB_RUN_SCHEDULER	= 0xfffffff	/* scheduler*/
};

/*
 * Some enumerated orders; "ANY" sorts last.
 */
enum sysinit_elem_order {
	SI_ORDER_FIRST		= 0x0000000,	/* first*/
	SI_ORDER_SECOND		= 0x0000001,	/* second*/
	SI_ORDER_THIRD		= 0x0000002,	/* third*/
	SI_ORDER_MIDDLE		= 0x1000000,	/* somewhere in the middle */
	SI_ORDER_ANY		= 0xfffffff	/* last*/
};
```
