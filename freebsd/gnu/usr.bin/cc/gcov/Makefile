# $FreeBSD$

.include "../Makefile.inc"

.PATH: ${GCCDIR} ${GCCDIR}/doc		${GCCDIR}/../libiberty

PROG=	gcov
SRCS=	gcov.c version.c

DPADD=	${LIBCC_INT}
LDADD=	${LIBCC_INT}

.include <bsd.prog.mk>
