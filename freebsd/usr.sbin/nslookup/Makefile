# $FreeBSD$

USE_LIBBIND=	yes
.include "${.CURDIR}/../named/Makefile.inc"

.PATH: ${BIND_DIR}/bin/nslookup ${BIND_DIR}/doc/man

PROG=	nslookup
MAN=	nslookup.8
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c commands.l

CFLAGS+= -D_PATH_HELPFILE=\"${DESTHELP}/nslookup.help\"
CFLAGS+= -I${.CURDIR}/../../contrib/bind/bin/nslookup

DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}
LDADD+=	-ll -ledit -ltermcap

FILES=	nslookup.help
FILESDIR=	${DESTHELP}

.include <bsd.prog.mk>
