# $FreeBSD$

PROG=	kldxref
MAN=	kldxref.8
SRCS=	kldxref.c ef.c

WARNS?=	2

.if exists(ef_${MACHINE_ARCH}.c)
SRCS+=	ef_${MACHINE_ARCH}.c
.else
SRCS+=	ef_nop.c
.endif

.include <bsd.prog.mk>
