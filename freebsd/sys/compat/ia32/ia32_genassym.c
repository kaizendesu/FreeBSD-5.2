#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_compat.h"

#include <sys/param.h>
#include <sys/assym.h>
#include <sys/systm.h>
#include <sys/signal.h>

#include <compat/ia32/ia32_signal.h>

ASSYM(IA32_SIGF_HANDLER, offsetof(struct ia32_sigframe, sf_ah));
ASSYM(IA32_SIGF_UC, offsetof(struct ia32_sigframe, sf_uc));
ASSYM(IA32_UC_GS, offsetof(struct ia32_ucontext, uc_mcontext.mc_gs));
ASSYM(IA32_UC_FS, offsetof(struct ia32_ucontext, uc_mcontext.mc_fs));
ASSYM(IA32_UC_ES, offsetof(struct ia32_ucontext, uc_mcontext.mc_es));
ASSYM(IA32_UC_DS, offsetof(struct ia32_ucontext, uc_mcontext.mc_ds));
#ifdef COMPAT_FREEBSD4
ASSYM(IA32_SIGF_UC4, offsetof(struct ia32_sigframe, sf_uc));
ASSYM(IA32_UC4_GS, offsetof(struct ia32_ucontext4, uc_mcontext.mc_gs));
ASSYM(IA32_UC4_FS, offsetof(struct ia32_ucontext4, uc_mcontext.mc_fs));
ASSYM(IA32_UC4_ES, offsetof(struct ia32_ucontext4, uc_mcontext.mc_es));
ASSYM(IA32_UC4_DS, offsetof(struct ia32_ucontext4, uc_mcontext.mc_ds));
#endif
