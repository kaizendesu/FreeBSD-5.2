# $FreeBSD$

.PATH: ${.CURDIR}/../../contrib/dev/acpica \
       ${.CURDIR}/../../pci \
       ${.CURDIR}/../../dev/acpica \
       ${.CURDIR}/../../dev/acpica/Osd \
       ${.CURDIR}/../../${MACHINE_ARCH}/acpica

KMOD=	acpi

# ACPI CA sources
CFLAGS+=   -I${.CURDIR}/../../contrib/dev/acpica
SRCS+=	dsfield.c dsinit.c dsmethod.c dsmthdat.c
SRCS+=	dsobject.c dsopcode.c dsutils.c dswexec.c dswload.c
SRCS+=	dswscope.c dswstate.c evevent.c evgpe.c evgpeblk.c
SRCS+=	evmisc.c evregion.c evrgnini.c evsci.c evxface.c
SRCS+=	evxfevnt.c evxfregn.c exconfig.c exconvrt.c excreate.c
SRCS+=	exdump.c exfield.c exfldio.c exmisc.c exmutex.c
SRCS+=	exnames.c exoparg1.c exoparg2.c exoparg3.c exoparg6.c
SRCS+=	exprep.c exregion.c exresnte.c exresolv.c exresop.c
SRCS+=	exstore.c exstoren.c exstorob.c exsystem.c exutils.c
SRCS+=	hwacpi.c hwgpe.c hwregs.c hwsleep.c hwtimer.c
SRCS+=	nsaccess.c nsalloc.c nsdump.c nseval.c nsinit.c
SRCS+=	nsload.c nsnames.c nsobject.c nsparse.c nssearch.c
SRCS+=	nsutils.c nswalk.c nsxfeval.c nsxfname.c nsxfobj.c
SRCS+=	psargs.c psopcode.c psparse.c psscope.c
SRCS+=	pstree.c psutils.c pswalk.c psxface.c
SRCS+=	rsaddr.c rscalc.c rscreate.c rsdump.c rsio.c
SRCS+=	rsirq.c rslist.c rsmemory.c rsmisc.c rsutils.c
SRCS+=	rsxface.c tbconvrt.c tbget.c tbgetall.c tbinstal.c
SRCS+=	tbrsdt.c tbutils.c tbxface.c tbxfroot.c utalloc.c
SRCS+=	utclib.c utcopy.c utdebug.c utdelete.c uteval.c
SRCS+=	utglobal.c utinit.c utmath.c utmisc.c utobject.c
SRCS+=	utxface.c

# OSD layer
SRCS+=	acpi.c acpi_acad.c acpi_battery.c acpi_button.c acpi_cmbat.c acpi_cpu.c
SRCS+=	acpi_ec.c acpi_isab.c acpi_lid.c
SRCS+=	acpi_package.c acpi_pci.c acpi_pcib.c acpi_pcib_acpi.c
SRCS+=	acpi_pcib_pci.c acpi_powerres.c acpi_resource.c acpi_thermal.c
SRCS+=	acpi_timer.c acpi_pci_link.c 
SRCS+=	OsdDebug.c 
SRCS+=	OsdHardware.c OsdInterrupt.c OsdMemory.c OsdSchedule.c
SRCS+=	OsdStream.c OsdSynch.c OsdTable.c OsdEnvironment.c 
SRCS+=	opt_acpi.h opt_bus.h opt_ddb.h
SRCS+=	device_if.h bus_if.h pci_if.h pcib_if.h isa_if.h
.if ACPI_MAX_THREADS
CFLAGS+=-DACPI_MAX_THREADS=${ACPI_MAX_THREADS}
.endif
.if ACPI_NO_SEMAPHORES
CFLAGS+=-DACPI_NO_SEMAPHORES
.endif

# Debugging support
.if ACPI_DEBUG
CFLAGS+=-DACPI_DEBUG
SRCS+=	dbcmds.c dbdisply.c dbexec.c dbfileio.c dbhistry.c
SRCS+=	dbinput.c dbstats.c dbutils.c dbxface.c
SRCS+=	dmbuffer.c dmnames.c dmopcode.c dmobject.c dmresrc.c dmresrcl.c
SRCS+=	dmresrcs.c dmutils.c dmwalk.c
opt_ddb.h: Makefile
	echo '#define DDB 1' > ${.TARGET}
.else
opt_ddb.h: Makefile
	echo -n > ${.TARGET}
.endif

# Machine-specific code such as sleep/wakeup
SRCS+=	acpi_machdep.c acpi_wakecode.h acpi_wakeup.c
.if ${MACHINE} == "i386"
SRCS+=	madt.c
.endif
CLEANFILES+=	acpi_wakecode.h acpi_wakecode.o acpi_wakecode.bin

acpi_wakecode.h: acpi_wakecode.S
	${MAKE} -f ${.CURDIR}/../../${MACHINE_ARCH}/acpica/Makefile \
		MAKESRCPATH=${.CURDIR}/../../${MACHINE_ARCH}/acpica

.include <bsd.kmod.mk>
