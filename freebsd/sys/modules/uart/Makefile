# $FreeBSD$

.PATH: ${.CURDIR}/../../dev/uart

.if ${MACHINE_ARCH} == "sparc64"
uart_bus_ebus=	uart_bus_ebus.c
.endif

KMOD=	uart
SRCS=	uart_bus_acpi.c ${uart_bus_ebus} uart_bus_isa.c uart_bus_pccard.c \
	uart_bus_pci.c uart_bus_puc.c uart_core.c uart_cpu_${MACHINE}.c \
	uart_dev_i8251.c uart_dev_ns8250.c uart_dev_sab82532.c \
	uart_dev_z8530.c uart_if.c uart_tty.c
SRCS+=	bus_if.h device_if.h isa_if.h pci_if.h uart_if.h power_if.h card_if.h

.include <bsd.kmod.mk>
