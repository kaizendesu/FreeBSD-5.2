#	@(#)Makefile	8.1 (Berkeley) 6/8/93
# $FreeBSD$

FILES1= dot.cshrc dot.login dot.login_conf dot.mailrc dot.profile dot.shrc
FILES2=	dot.mail_aliases dot.rhosts 
MODE1=	0644
MODE2=	0600

NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m ${MODE1} ${FILES1} \
	    ${DESTDIR}${BINDIR}/skel
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m ${MODE2} ${FILES2} \
	    ${DESTDIR}${BINDIR}/skel

.include <bsd.prog.mk>
