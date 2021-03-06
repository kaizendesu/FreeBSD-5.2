# $FreeBSD$
#
LIB=			i386
INTERNALLIB=		true

SRCS=	biosacpi.c bioscd.c biosdisk.c biosmem.c biospnp.c \
	biospci.c biossmap.c bootinfo.c bootinfo32.c bootinfo64.c \
	comconsole.c devicename.c elf32_freebsd.c \
	elf64_freebsd.c gatea20.c \
	i386_copy.c i386_module.c nullconsole.c pxe.c pxetramp.s \
	time.c vidconsole.c amd64_tramp.S

CFLAGS+=	-ffreestanding
BOOT_COMCONSOLE_PORT?= 0x3f8
CFLAGS+=	-DCOMPORT=${BOOT_COMCONSOLE_PORT}

BOOT_COMCONSOLE_SPEED?= 9600
CFLAGS+=	-DCOMSPEED=${BOOT_COMCONSOLE_SPEED}

.ifdef(BOOT_BIOSDISK_DEBUG)
# Make the disk code more talkative
CFLAGS+= -DDISK_DEBUG
.endif

# Include simple terminal emulation (cons25-compatible)
CFLAGS+= -DTERM_EMU

CFLAGS+=	-I${.CURDIR}/../../common -I${.CURDIR}/../btx/lib \
		-I${.CURDIR}/../../../contrib/dev/acpica \
		-I${.CURDIR}/../../.. -I.
# the location of libstand
CFLAGS+=	-I${.CURDIR}/../../../../lib/libstand/

# Make "machine" required for all objects
# (based on the more complete case in sys/i386/boot/Makefile.inc)
${SRCS:M*.c:R:S/$/.o/g}: machine

# If it's not there, don't consider it a target
.if exists(${.CURDIR}/../../../i386/include)
beforedepend ${OBJS}: machine

CLEANFILES+=	machine
machine:
	ln -sf ${.CURDIR}/../../../i386/include machine

.endif

.include <bsd.lib.mk>
