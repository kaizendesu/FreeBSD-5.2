# $FreeBSD$

PROG=	kdestroy

SRCS=	\
	kdestroy.c		\
	${.OBJDIR}/krb5_err.h	\
	${.OBJDIR}/heim_err.h	\
	${.OBJDIR}/k524_err.h

CFLAGS+=-I${KRB5DIR}/include	\
	-I${KRB5DIR}/lib/roken	\
	-I${KRB5DIR}/lib/kafs	\
	-I${KRB5DIR}/lib/krb5	\
	-I${KRB5DIR}/lib/asn1	\
	-I${KRB5DIR}/kuser	\
	-I${ASN1OBJDIR}		\
	-I${.OBJDIR}

LDADD=	-lkrb5 -lroken -lkafs5 ${LIBVERS} \
	-lasn1 -lcrypto -lcrypt -lcom_err

DPADD=	${LIBKRB5} ${LIBROKEN} ${LIBKAFS5} ${LIBVERS} \
	${LIBASN1} ${LIBCRYPTO} ${LIBCRYPT} ${LIBCOM_ERR}

.include <bsd.prog.mk>

.PATH:	${KRB5DIR}/kuser
