# $FreeBSD$

GCCDIR=	${.CURDIR}/../../../contrib/gcc
SRCDIR=	${.CURDIR}/../../../contrib/libstdc++/libsupc++

.PATH: ${SRCDIR} ${GCCDIR}

# Static only.
LIB=	supc++
SRCS=	del_op.cc del_opnt.cc del_opv.cc del_opvnt.cc \
	eh_alloc.cc eh_aux_runtime.cc eh_catch.cc eh_exception.cc \
	eh_globals.cc eh_personality.cc eh_terminate.cc eh_throw.cc eh_type.cc \
	guard.cc new_handler.cc new_op.cc new_opnt.cc new_opv.cc new_opvnt.cc \
	pure.cc tinfo.cc tinfo2.cc vec.cc

# from libiberty:
SRCS+=	cp-demangle.c dyn-string.c

CFLAGS+=	-DIN_GLIBCPP_V3 -DHAVE_CONFIG_H
CFLAGS+=	-I${SRCDIR} -I${GCCDIR} -I${.CURDIR}/../libstdc++
CXXFLAGS+=	-fno-implicit-templates -ffunction-sections -fdata-sections

HDRS=	exception new typeinfo cxxabi.h exception_defines.h
INCS=	${HDRS:S;^;${SRCDIR}/;}
INCSDIR=${INCLUDEDIR}/c++/3.3

.include <bsd.lib.mk>
