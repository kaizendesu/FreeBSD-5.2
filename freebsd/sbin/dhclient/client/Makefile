# client
# $FreeBSD$

DIST_DIR= ${.CURDIR}/../../../contrib/isc-dhcp

.PATH: ${DIST_DIR}/client

PROG=	dhclient
SRCS=	clparse.c dhclient.c

.if defined(RESCUE)
CFLAGS+= -DCLIENT_PATH='"PATH=/rescue:/sbin:/bin:/usr/sbin:/usr/bin"'
.else
CFLAGS+= -DCLIENT_PATH='"PATH=/sbin:/bin:/usr/sbin:/usr/bin"'
.endif
CFLAGS+= -Dwarn=dhcp_warn

DPADD=	${LIBDHCP} ${LIBRES} ${LIBOMAPI} ${LIBDST}
LDADD=	${LIBDHCP} ${LIBRES} ${LIBOMAPI} ${LIBDST} 

SCRIPTS=	scripts/freebsd
SCRIPTSNAME=	dhclient-script

MAN=	dhclient.conf.5 dhclient.leases.5
MAN+=	dhclient.8 dhclient-script.8

.include <bsd.prog.mk>
