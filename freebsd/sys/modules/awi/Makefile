# $FreeBSD$

.PATH: ${.CURDIR}/../../dev/awi

KMOD=	if_awi
SRCS=	am79c930.c awi.c awi_wep.c awi_wicfg.c if_awi_pccard.c
SRCS+=	bus_if.h card_if.h device_if.h opt_inet.h

opt_inet.h:
	echo "#define INET 1" > opt_inet.h

.include <bsd.kmod.mk>
