# $FreeBSD$

.include "../Makefile.inc0"

GDBDIR=	${.CURDIR}/../../../../contrib/gdb
CONTRIBDIR= ${.CURDIR}/../../../../contrib

.PATH: ${SRCDIR}/gas/doc ${SRCDIR}/ld ${SRCDIR}/bfd/doc ${GDBDIR}/gdb/doc ${GDBDIR}/gdb/mi

INFO = as ld gasp gdb gdbint stabs binutils
INFOSECTION=	"Programming & development tools."
INFOENTRY_as=	"* As: (as).            The GNU assembler."
INFOENTRY_gasp=	"* Gasp: (gasp).        The GNU Assembler Macro Preprocessor."
INFOENTRY_ld=	"* Ld: (ld).		The GNU linker."
INFOENTRY_binutils= "* Binutils: (binutils).		The GNU Binary Utilities."

MAKEINFOFLAGS+= --no-validate
MAKEINFOFLAGS+= -I ${SRCDIR}/gas/doc -I ${SRCDIR}/ld -I ${SRCDIR}/bfd/doc
MAKEINFOFLAGS+= -I ${SRCDIR}/binutils
MAKEINFOFLAGS+= -I ${GDBDIR}/gdb/doc
MAKEINFOFLAGS+= -I ${GDBDIR}/gdb/mi
MAKEINFOFLAGS+= -I ${CONTRIBDIR}/libreadline/doc

CLEANFILES=	configdoc.texi gdb-cfg.texi GDBvn.texi \
		inc-hist.texinfo inc-hist.texinfo.orig \
		config.texi gasver.texi ldver.texi

as.info:	as.texinfo asconfig.texi c-i386.texi gasver.texi
ld.info:	ld.texinfo bfdsumm.texi ldver.texi configdoc.texi

configdoc.texi: gen-doc.texi
	ln -sf ${.ALLSRC} ${.TARGET}

gdb.info: gdb.texinfo gdb-cfg.texi GDBvn.texi annotate.texi \
	  fdl.texi gpl.texi gdbmi.texinfo \
	rluser.texinfo inc-hist.texinfo

gdbint.info: gdbint.texinfo gdb-cfg.texi fdl.texi
	
gdb-cfg.texi: all-cfg.texi
	ln -sf ${.ALLSRC} ${.TARGET}

GDBvn.texi: ${GDBDIR}/gdb/version.in
	echo "@set GDBVN `sed q ${.ALLSRC}`" > ${.TARGET}

.PATH: ${SRCDIR}/binutils/doc
binutils.info: binutils.texi config.texi

config.texi gasver.texi ldver.texi:
	echo "@set VERSION ${VERSION}" > ${.TARGET}

.PATH: ${CONTRIBDIR}/libreadline/doc
inc-hist.texinfo: hsuser.texinfo inc-hist.diff
	cp ${.ALLSRC:M*.texinfo} ${.TARGET}
	patch -b .orig < ${.ALLSRC:M*.diff}

.include <bsd.info.mk>
