#	from: Makefile.inc,v 1.1 1993/09/03 19:04:23 jtc Exp
# $FreeBSD$

SRCS+=	amd64_get_fsbase.c amd64_get_gsbase.c amd64_set_fsbase.c amd64_set_gsbase.c

MDASM=	vfork.S brk.S cerror.S exect.S getcontext.S pipe.S ptrace.S \
	reboot.S sbrk.S setlogin.S sigreturn.S

# Don't generate default code for these syscalls:
NOASM=	break.o exit.o ftruncate.o getdomainname.o getlogin.o \
	lseek.o mmap.o openbsd_poll.o pread.o \
	pwrite.o setdomainname.o sstk.o truncate.o uname.o vfork.o yield.o

PSEUDO=	_getlogin.o _exit.o
