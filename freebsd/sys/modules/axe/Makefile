# $FreeBSD$

S=	${.CURDIR}/../..
.PATH: $S/dev/usb

KMOD=	if_axe
SRCS=	if_axe.c opt_bdg.h opt_usb.h device_if.h bus_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
