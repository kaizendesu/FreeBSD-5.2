# $FreeBSD$

PROG=		ldd
SRCS=		ldd.c
.if ${MACHINE_ARCH} == "i386"
SRCS+=		sods.c
.endif

.include <bsd.prog.mk>
