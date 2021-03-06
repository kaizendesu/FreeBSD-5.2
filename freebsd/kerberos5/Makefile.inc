# $FreeBSD$

DISTRIBUTION?=	crypto
NOLINT=		true

.if !defined(INCLUDEOBJDIR)

KRB5DIR=	${.CURDIR}/../../../crypto/heimdal
ROKENDIR=	${.CURDIR}/../../lib/libroken

CFLAGS+=-I${INCLUDEOBJDIR} -DHAVE_CONFIG_H

CFLAGS+=-DINET6

.if defined(WITH_OPENLDAP)
OPENLDAPBASE?=	/usr/local
LDAPLIBS=-lldap -llber
LDAPCFLAGS=-I${OPENLDAPBASE}/include -DOPENLDAP=1
LDAPLDADD=-L${OPENLDAPBASE}/lib ${LDAPLIBS}
.endif

.if exists(${.OBJDIR}/../../include)
INCLUDEOBJDIR=	${.OBJDIR}/../../include
.else
INCLUDEOBJDIR=	${.CURDIR}/../../include
.endif

.if exists(${.OBJDIR}/../../lib/libasn1)
ASN1OBJDIR=	${.OBJDIR}/../../lib/libasn1
.else
ASN1OBJDIR=	${.CURDIR}/../../lib/libasn1
.endif

.if exists(${.OBJDIR}/../../lib/libhdb)
HDBOBJDIR=	${.OBJDIR}/../../lib/libhdb
.else
HDBOBJDIR=	${.CURDIR}/../../lib/libhdb
.endif

.if exists(${.OBJDIR}/../../lib/libroken)
ROKENOBJDIR=	${.OBJDIR}/../../lib/libroken
.else
ROKENOBJDIR=	${.CURDIR}/../../lib/libroken
.endif

.if exists(${.OBJDIR}/../../lib/libvers)
LIBVERS=	${.OBJDIR}/../../lib/libvers/libvers.a
.else
LIBVERS=	${.CURDIR}/../../lib/libvers/libvers.a
.endif

.if exists(${.OBJDIR}/../../lib/libkrb5)
KRB5OBJDIR=	${.OBJDIR}/../../lib/libkrb5
.else
KRB5OBJDIR=	${.CURDIR}/../../lib/libkrb5
.endif

.if exists(${.OBJDIR}/../../lib/libsl)
LIBSL=		${.OBJDIR}/../../lib/libsl/libsl.a
.else
LIBSL=		${.CURDIR}/../../lib/libsl/libsl.a
.endif

COMPILE_ET=     compile_et

.ORDER: asn1_err.c ${.OBJDIR}/asn1_err.h
asn1_err.c ${.OBJDIR}/asn1_err.h: ${KRB5DIR}/lib/asn1/asn1_err.et
	test -e ${.OBJDIR}/asn1_err.et || ln -sf ${.ALLSRC}
	${COMPILE_ET} asn1_err.et

CLEANFILES+=asn1_err.h asn1_err.c asn1_err.et

.ORDER: hdb_err.c ${.OBJDIR}/hdb_err.h
hdb_err.c ${.OBJDIR}/hdb_err.h: ${KRB5DIR}/lib/hdb/hdb_err.et
	test -e ${.OBJDIR}/hdb_err.et || ln -sf ${.ALLSRC}
	${COMPILE_ET} hdb_err.et

CLEANFILES+=hdb_err.h hdb_err.c hdb_err.et

.ORDER: heim_err.c ${.OBJDIR}/heim_err.h
heim_err.c ${.OBJDIR}/heim_err.h: ${KRB5DIR}/lib/krb5/heim_err.et
	test -e ${.OBJDIR}/heim_err.et || ln -sf ${.ALLSRC}
	${COMPILE_ET} heim_err.et

CLEANFILES+=heim_err.h heim_err.c heim_err.et

.ORDER: krb5_err.c ${.OBJDIR}/krb5_err.h
krb5_err.c ${.OBJDIR}/krb5_err.h: ${KRB5DIR}/lib/krb5/krb5_err.et
	test -e ${.OBJDIR}/krb5_err.et || ln -sf ${.ALLSRC}
	${COMPILE_ET} krb5_err.et

CLEANFILES+=krb5_err.h krb5_err.c krb5_err.et

.ORDER: kadm5_err.c ${.OBJDIR}/kadm5_err.h
kadm5_err.c ${.OBJDIR}/kadm5_err.h: \
	${KRB5DIR}/lib/kadm5/kadm5_err.et
	test -e ${.OBJDIR}/kadm5_err.et || ln -sf ${.ALLSRC}
	${COMPILE_ET} kadm5_err.et

CLEANFILES+=kadm5_err.h kadm5_err.c kadm5_err.et

.ORDER: k524_err.c ${.OBJDIR}/k524_err.h
k524_err.c ${.OBJDIR}/k524_err.h: \
	${KRB5DIR}/lib/krb5/k524_err.et
	test -e ${.OBJDIR}/k524_err.et || ln -sf ${.ALLSRC}
	${COMPILE_ET} k524_err.et

CLEANFILES+=k524_err.h k524_err.c k524_err.et

${.OBJDIR}/roken.h: make-roken
	./make-roken > tmp.h ;\
	if [ -f roken.h ] && cmp -s tmp.h roken.h ; then rm -f tmp.h ; \
	else rm -f roken.h; mv tmp.h roken.h; fi

make-roken.c: ${KRB5DIR}/lib/roken/roken.awk ${KRB5DIR}/lib/roken/roken.h.in
	awk -f ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	make-roken.c make-roken roken.h

.else

KRB5DIR=	${.CURDIR}/../../crypto/heimdal
ROKENDIR=	${.CURDIR}/../../lib/libroken

.endif
