# $FreeBSD$

PROG=	MdLoad

LDADD=	-lsbuf -lbsdxml

NOMAN=	sorry


WARNS=4
.include "bsd.prog.mk"

test:	${PROG}
	./${PROG} md34 ../Data/disk.critter.ad0.xml
