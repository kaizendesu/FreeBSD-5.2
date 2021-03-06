# $FreeBSD$

.include "../Makefile.inc"

.PATH: ../cc_tools ${GCCDIR}/config/${GCC_CPU} ${GCCDIR} ${GCCDIR}/../libiberty

LIB=		cc_int
INTERNALLIB=	YES

#	dbxout.c dwarfout.c emit-rtl.c explow.c expmed.c expr.c \
#	alias.c bitmap.c dwarf2out.c dyn-string.c \
SRCS=	c-common.c c-dump.c c-format.c c-opts.c \
	c-pragma.c c-semantics.c \
	caller-save.c calls.c combine.c convert.c cse.c \
	dbxout.c dwarfout.c dwarf2out.c dwarf2asm.c emit-rtl.c explow.c expmed.c expr.c \
	final.c flow.c fold-const.c function.c getpwd.c global.c graph.c \
	gtype-desc.c hash.c haifa-sched.c \
	integrate.c jump.c lcm.c local-alloc.c loop.c optabs.c \
	print-rtl.c print-tree.c \
	ra.c ra-build.c ra-colorize.c ra-debug.c ra-rewrite.c \
	real.c recog.c reg-stack.c regclass.c \
	reload.c reload1.c reorg.c rtl.c rtlanal.c \
	sbitmap.c sdbout.c stmt.c stor-layout.c \
	toplev.c tracer.c tree.c unroll.c varasm.c version.c xcoffout.c \
	alias.c bitmap.c dyn-string.c \
	gcse.c genrtl.c profile.c regmove.c varray.c \
	attribs.c cselib.c debug.c rtl-error.c tree-dump.c tree-inline.c

SRCS+=	${GCC_CPU}.c
.if exists(${GCCDIR}/config/${GCC_CPU}/${GCC_CPU}-c.c)
SRCS+=	${GCC_CPU}-c.c
.endif

SRCS+=	bb-reorder.c conflict.c ggc-common.c \
	ggc-page.c ifcvt.c lists.c predict.c regrename.c resource.c sibcall.c \
	simplify-rtx.c ssa.c timevar.c ssa-dce.c ssa-ccp.c df.c fibheap.c

# insn-* gunk -- headers are built in cc_tools, as they are used by the
# "build-tools"
.for F in attrtab emit extract opinit output peep recog
SRCS+=	insn-$F.c
CLEANFILES+= insn-$F.c
insn-$F.c: ${.OBJDIR}/../cc_tools/gen$F ${MD_FILE}
	${.OBJDIR}/../cc_tools/gen$F ${MD_FILE} > insn-$F.c
.endfor

#	shared between the drivers
SRCS+=	multilib.h choose-temp.c obstack.c prefix.c pexecute.c version.c mbchar.c concat.c make-temp-file.c underscore.c
#	C_AND_OBJC_OBJS
#SRCS+=	attribs.o c-errors.o c-lex.o c-pragma.o c-decl.o c-typeck.o \
#	c-convert.o c-aux-info.o c-common.o c-format.o c-semantics.o \
#	c-objc-common.o

#	XXX: shared by cc1, cc1obj & cpp0
#	cc1plus seems to be able to tolerate these here,
#	BUT only if the right .o's are specified in cc1plus/Makefile
SRCS+=	c-lex.c c-typeck.c \
		c-errors.o c-pragma.o c-convert.o c-aux-info.o c-common.o c-format.o c-semantics.o c-objc-common.o

#	libbackend
SRCS+=	builtins.c diagnostic.c doloop.c dominance.c et-forest.c \
	except.c gcse.c params.c sched-deps.c sched-vis.c sched-rgn.c sched-ebb.c \
	stringpool.c c-lex.c langhooks.c hashtable.c \
	hooks.c \
	cfg.c cfganal.c cfgbuild.c cfgcleanup.c cfglayout.c cfgloop.c cfgrtl.c

#	libcpp
SRCS+=	cppdefault.c cpperror.c cppexp.c cppfiles.c cppinit.c cpphash.c \
	cpplex.c cpplib.c cppmacro.c cppmain.c cpptrad.c mkdeps.c line-map.c

#	stuff that makes libiberty now required (2.95) and shouldn't be...
SRCS+=	obstack.c splay-tree.c hashtab.c partition.c lbasename.c md5.c hex.c getopt.c getopt1.c xmemdup.c physmem.c
CFLAGS+=	-DHAVE_CONFIG_H

CFLAGS+=	-DTARGET_NAME=\"${target}\" -DIN_GCC
.if defined(TARGET_CPU_DEFAULT)
CFLAGS+=	-DTARGET_CPU_DEFAULT=${TARGET_CPU_DEFAULT}
.endif

UNDERSCORES=	0
SRCS+=	underscore.c
CLEANFILES+=	underscore.c
underscore.c: Makefile
	echo '/*WARNING: This file is automatically generated!*/' >${.TARGET}
	echo "int prepends_underscore = ${UNDERSCORES};"	  >>${.TARGET}

.include <bsd.lib.mk>
