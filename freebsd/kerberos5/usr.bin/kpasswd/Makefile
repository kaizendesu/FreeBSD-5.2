# $FreeBSD$

PROG=	kpasswd

SRCS=	\
	kpasswd.c		\
	${.OBJDIR}/krb5_err.h	\
	${.OBJDIR}/heim_err.h	\
	${.OBJDIR}/k524_err.h

CFLAGS+=-I${KRB5DIR}/include	\
	-I${KRB5DIR}/lib/asn1	\
	-I${KRB5DIR}/lib/krb5	\
	-I${KRB5DIR}/lib/roken	\
	-I${KRB5DIR}/kpasswd	\
	-I${ASN1OBJDIR}		\
	-I${.OBJDIR}

LDADD=	-lkrb5 -lroken ${LIBVERS} \
	-lasn1 -lcrypto -lcrypt -lcom_err

DPADD=	${LIBKRB5} ${LIBROKEN} ${LIBVERS} \
	${LIBASN1} ${LIBCRYPTO} ${LIBCRYPT} ${LIBCOM_ERR}

.include <bsd.prog.mk>

.PATH:	${KRB5DIR}/kpasswd
