# $FreeBSD$

MAINTAINER=	darrenr@freebsd.org

.PATH: ${.CURDIR}/../../contrib/ipfilter/ipsend \
       ${.CURDIR}/../../contrib/ipfilter

PROG=	iptest
SRCS=	iptest.c iptests.c ip.c sbpf.c 44arp.c sock.c

CFLAGS+= -DDOSOCKET -DIPL_NAME=\"/dev/ipl\" -DUSE_INET6
CFLAGS+= -I${.CURDIR}/../../sys/contrib/ipfilter/netinet
CFLAGS+= -I${.CURDIR}/../../contrib/ipfilter/ipsend \
	 -I${.CURDIR}/../../contrib/ipfilter

.include <bsd.prog.mk>
