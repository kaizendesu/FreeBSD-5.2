/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#if 0
#ifndef lint
static char sccsid[] = "@(#)ps.c	8.4 (Berkeley) 4/2/94";
#endif /* not lint */
#endif

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <locale.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ps.h"

#define SEP ", \t"		/* username separators */

static KINFO *kinfo;
struct varent *vhead;

int	eval;			/* exit value */
int	cflag;			/* -c */
int	rawcpu;			/* -C */
int	sumrusage;		/* -S */
int	termwidth;		/* width of screen (0 == infinity) */
int	totwidth;		/* calculated width of requested variables */

time_t	now;			/* current time(3) value */

static int needuser, needcomm, needenv;
#if defined(LAZY_PS)
static int forceuread=0;
#else
static int forceuread=1;
#endif

static enum sort { DEFAULT, SORTMEM, SORTCPU } sortby = DEFAULT;

static const	 char *fmt(char **(*)(kvm_t *, const struct kinfo_proc *, int),
		    KINFO *, char *, int);
static char	*kludge_oldps_options(char *);
static int	 pscomp(const void *, const void *);
static void	 saveuser(KINFO *);
static void	 scanvars(void);
static void	 dynsizevars(KINFO *);
static void	 sizevars(void);
static void	 usage(void);
static uid_t	*getuids(const char *, int *);

static char dfmt[] = "pid,tt,state,time,command";
static char jfmt[] = "user,pid,ppid,pgid,jobc,state,tt,time,command";
static char lfmt[] = "uid,pid,ppid,cpu,pri,nice,vsz,rss,mwchan,state,tt,time,command";
static char   o1[] = "pid";
static char   o2[] = "tt,state,time,command";
static char ufmt[] = "user,pid,%cpu,%mem,vsz,rss,tt,state,start,time,command";
static char vfmt[] = "pid,state,time,sl,re,pagein,vsz,rss,lim,tsiz,%cpu,%mem,command";
static char Zfmt[] = "label";

static kvm_t *kd;

#if defined(LAZY_PS)
#define PS_ARGS	"aCcefgHhjLlM:mN:O:o:p:rSTt:U:uvwxZ"
#else
#define PS_ARGS	"aCcegHhjLlM:mN:O:o:p:rSTt:U:uvwxZ"
#endif

int
main(int argc, char *argv[])
{
	struct kinfo_proc *kp;
	struct varent *vent;
	struct winsize ws;
	dev_t ttydev;
	pid_t pid;
	uid_t *uids;
	int all, ch, flag, i, _fmt, lineno, nentries, nocludge, dropgid;
	int prtheader, wflag, what, xflg, uid, nuids, showthreads;
	char *cols;
	char errbuf[_POSIX2_LINE_MAX];
	const char *cp, *nlistf, *memf;

	(void) setlocale(LC_ALL, "");
	/* Set the time to what it is right now. */
	time(&now);

	if ((cols = getenv("COLUMNS")) != NULL && *cols != '\0')
		termwidth = atoi(cols);
	else if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDERR_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDIN_FILENO,  TIOCGWINSZ, (char *)&ws) == -1) ||
	     ws.ws_col == 0)
		termwidth = 79;
	else
		termwidth = ws.ws_col - 1;

	/*
	 * Don't apply a kludge if the first argument is an option taking an
	 * argument
	 */
	if (argc > 1) {
		nocludge = 0;
		if (argv[1][0] == '-') {
			for (cp = PS_ARGS; *cp != '\0'; cp++) {
				if (*cp != ':')
					continue;
				if (*(cp - 1) == argv[1][1]) {
					nocludge = 1;
					break;
				}
			}
		}
		if (nocludge == 0)
			argv[1] = kludge_oldps_options(argv[1]);
	}

	all = _fmt = prtheader = wflag = xflg = 0;
	pid = -1;
	nuids = 0;
	uids = NULL;
	ttydev = NODEV;
	dropgid = 0;
	memf = nlistf = _PATH_DEVNULL;
	showthreads = 0;
	while ((ch = getopt(argc, argv, PS_ARGS)) != -1)
		switch((char)ch) {
		case 'a':
			all = 1;
			break;
		case 'C':
			rawcpu = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'e':			/* XXX set ufmt */
			needenv = 1;
			break;
		case 'g':
			break;			/* no-op */
		case 'H':
			showthreads = 1;
			break;
		case 'h':
			prtheader = ws.ws_row > 5 ? ws.ws_row : 22;
			break;
		case 'j':
			parsefmt(jfmt, 0);
			_fmt = 1;
			jfmt[0] = '\0';
			break;
		case 'L':
			showkey();
			exit(0);
		case 'l':
			parsefmt(lfmt, 0);
			_fmt = 1;
			lfmt[0] = '\0';
			break;
		case 'M':
			memf = optarg;
			dropgid = 1;
			break;
		case 'm':
			sortby = SORTMEM;
			break;
		case 'N':
			nlistf = optarg;
			dropgid = 1;
			break;
		case 'O':
			parsefmt(o1, 1);
			parsefmt(optarg, 1);
			parsefmt(o2, 1);
			o1[0] = o2[0] = '\0';
			_fmt = 1;
			break;
		case 'o':
			parsefmt(optarg, 1);
			_fmt = 1;
			break;
#if defined(LAZY_PS)
		case 'f':
			if (getuid() == 0 || getgid() == 0)
			    forceuread = 1;
			break;
#endif
		case 'p':
			pid = atol(optarg);
			xflg = 1;
			break;
		case 'r':
			sortby = SORTCPU;
			break;
		case 'S':
			sumrusage = 1;
			break;
		case 'T':
			if ((optarg = ttyname(STDIN_FILENO)) == NULL)
				errx(1, "stdin: not a terminal");
			/* FALLTHROUGH */
		case 't': {
			struct stat sb;
			char *ttypath, pathbuf[PATH_MAX];

			if (strcmp(optarg, "co") == 0)
				ttypath = strdup(_PATH_CONSOLE);
			else if (*optarg != '/')
				(void)snprintf(ttypath = pathbuf,
				    sizeof(pathbuf), "%s%s", _PATH_TTY, optarg);
			else
				ttypath = optarg;
			if (stat(ttypath, &sb) == -1)
				err(1, "%s", ttypath);
			if (!S_ISCHR(sb.st_mode))
				errx(1, "%s: not a terminal", ttypath);
			ttydev = sb.st_rdev;
			break;
		}
		case 'U':
			uids = getuids(optarg, &nuids);
			xflg++;		/* XXX: intuitive? */
			break;
		case 'u':
			parsefmt(ufmt, 0);
			sortby = SORTCPU;
			_fmt = 1;
			ufmt[0] = '\0';
			break;
		case 'v':
			parsefmt(vfmt, 0);
			sortby = SORTMEM;
			_fmt = 1;
			vfmt[0] = '\0';
			break;
		case 'w':
			if (wflag)
				termwidth = UNLIMITED;
			else if (termwidth < 131)
				termwidth = 131;
			wflag++;
			break;
		case 'x':
			xflg = 1;
			break;
		case 'Z':
			parsefmt(Zfmt, 0);
			Zfmt[0] = '\0';
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		nlistf = *argv;
		if (*++argv) {
			memf = *argv;
		}
	}
#endif
	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (dropgid) {
		setgid(getgid());
		setuid(getuid());
	}

	kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf);
	if (kd == 0)
		errx(1, "%s", errbuf);

	if (!_fmt)
		parsefmt(dfmt, 0);

	/* XXX - should be cleaner */
	if (!all && ttydev == NODEV && pid == -1 && !nuids) {
		if ((uids = malloc(sizeof (*uids))) == NULL)
			errx(1, "malloc failed");
		nuids = 1;
		*uids = getuid();
	}

	/*
	 * scan requested variables, noting what structures are needed,
	 * and adjusting header widths as appropriate.
	 */
	scanvars();
	/*
	 * get proc list
	 */
	if (nuids == 1) {
		what = KERN_PROC_UID;
		flag = *uids;
	} else if (ttydev != NODEV) {
		what = KERN_PROC_TTY;
		flag = ttydev;
	} else if (pid != -1) {
		what = KERN_PROC_PID;
		flag = pid;
	} else if (showthreads == 1) {
		what = KERN_PROC_ALL;
		flag = 0;
	} else {
		what = KERN_PROC_PROC;
		flag = 0;
	}
	/*
	 * select procs
	 */
	if ((kp = kvm_getprocs(kd, what, flag, &nentries)) == 0 || nentries < 0)
		errx(1, "%s", kvm_geterr(kd));
	if ((kinfo = malloc(nentries * sizeof(*kinfo))) == NULL)
		errx(1, "malloc failed");
	for (i = nentries; --i >= 0; ++kp) {
		kinfo[i].ki_p = kp;
		if (needuser)
			saveuser(&kinfo[i]);
		dynsizevars(&kinfo[i]);
	}

	sizevars();

	/*
	 * print header
	 */
	printheader();
	if (nentries == 0)
		exit(1);
	/*
	 * sort proc list
	 */
	qsort(kinfo, nentries, sizeof(KINFO), pscomp);
	/*
	 * for each proc, call each variable output function.
	 */
	for (i = lineno = 0; i < nentries; i++) {
		if (xflg == 0 && ((&kinfo[i])->ki_p->ki_tdev == NODEV ||
		    ((&kinfo[i])->ki_p->ki_flag & P_CONTROLT ) == 0))
			continue;
		if (nuids > 1) {
			for (uid = 0; uid < nuids; uid++)
				if ((&kinfo[i])->ki_p->ki_uid == uids[uid])
					break;
			if (uid == nuids)
				continue;
		}
		for (vent = vhead; vent; vent = vent->next) {
			(vent->var->oproc)(&kinfo[i], vent);
			if (vent->next != NULL)
				(void)putchar(' ');
		}
		(void)putchar('\n');
		if (prtheader && lineno++ == prtheader - 4) {
			(void)putchar('\n');
			printheader();
			lineno = 0;
		}
	}
	free(uids);

	exit(eval);
}

uid_t *
getuids(const char *arg, int *nuids)
{
	char name[MAXLOGNAME];
	struct passwd *pwd;
	uid_t *uids, *moreuids;
	int alloc;
	size_t l;


	alloc = 0;
	*nuids = 0;
	uids = NULL;
	for (; (l = strcspn(arg, SEP)) > 0; arg += l + strspn(arg + l, SEP)) {
		if (l >= sizeof name) {
			warnx("%.*s: name too long", (int)l, arg);
			continue;
		}
		strncpy(name, arg, l);
		name[l] = '\0';
		if ((pwd = getpwnam(name)) == NULL) {
			warnx("%s: no such user", name);
			continue;
		}
		if (*nuids >= alloc) {
			alloc = (alloc + 1) << 1;
			moreuids = realloc(uids, alloc * sizeof (*uids));
			if (moreuids == NULL) {
				free(uids);
				errx(1, "realloc failed");
			}
			uids = moreuids;
		}
		uids[(*nuids)++] = pwd->pw_uid;
	}
	endpwent();

	if (!*nuids)
		errx(1, "No users specified");

	return uids;
}

VARENT *
find_varentry(VAR *v)
{
	struct varent *vent;

	for (vent = vhead; vent; vent = vent->next) {
		if (strcmp(vent->var->name, v->name) == 0)
			return vent;
	}
	return NULL;
}

static void
scanvars(void)
{
	struct varent *vent;
	VAR *v;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		if (v->flag & DSIZ) {
			v->dwidth = v->width;
			v->width = 0;
		}
		if (v->flag & USER)
			needuser = 1;
		if (v->flag & COMM)
			needcomm = 1;
	}
}

static void
dynsizevars(KINFO *ki)
{
	struct varent *vent;
	VAR *v;
	int i;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		if (!(v->flag & DSIZ))
			continue;
		i = (v->sproc)( ki);
		if (v->width < i)
			v->width = i;
		if (v->width > v->dwidth)
			v->width = v->dwidth;
	}
}

static void
sizevars(void)
{
	struct varent *vent;
	VAR *v;
	int i;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		i = strlen(vent->header);
		if (v->width < i)
			v->width = i;
		totwidth += v->width + 1;	/* +1 for space */
	}
	totwidth--;
}

static const char *
fmt(char **(*fn)(kvm_t *, const struct kinfo_proc *, int), KINFO *ki,
  char *comm, int maxlen)
{
	const char *s;

	s = fmt_argv((*fn)(kd, ki->ki_p, termwidth), comm, maxlen);
	return (s);
}

#define UREADOK(ki)	(forceuread || (ki->ki_p->ki_sflag & PS_INMEM))

static void
saveuser(KINFO *ki)
{

	if (ki->ki_p->ki_sflag & PS_INMEM) {
		/*
		 * The u-area might be swapped out, and we can't get
		 * at it because we have a crashdump and no swap.
		 * If it's here fill in these fields, otherwise, just
		 * leave them 0.
		 */
		ki->ki_valid = 1;
	} else
		ki->ki_valid = 0;
	/*
	 * save arguments if needed
	 */
	if (needcomm && (UREADOK(ki) || (ki->ki_p->ki_args != NULL))) {
		ki->ki_args = strdup(fmt(kvm_getargv, ki, ki->ki_p->ki_comm,
		    MAXCOMLEN));
	} else if (needcomm) {
		asprintf(&ki->ki_args, "(%s)", ki->ki_p->ki_comm);
	} else {
		ki->ki_args = NULL;
	}
	if (needenv && UREADOK(ki)) {
		ki->ki_env = strdup(fmt(kvm_getenvv, ki, (char *)NULL, 0));
	} else if (needenv) {
		ki->ki_env = malloc(3);
		strcpy(ki->ki_env, "()");
	} else {
		ki->ki_env = NULL;
	}
}

static int
pscomp(const void *a, const void *b)
{
	int i;
#define VSIZE(k) ((k)->ki_p->ki_dsize + (k)->ki_p->ki_ssize + \
		  (k)->ki_p->ki_tsize)

	if (sortby == SORTCPU)
		return (getpcpu((const KINFO *)b) - getpcpu((const KINFO *)a));
	if (sortby == SORTMEM)
		return (VSIZE((const KINFO *)b) - VSIZE((const KINFO *)a));
	i =  (int)((const KINFO *)a)->ki_p->ki_tdev - (int)((const KINFO *)b)->ki_p->ki_tdev;
	if (i == 0)
		i = ((const KINFO *)a)->ki_p->ki_pid - ((const KINFO *)b)->ki_p->ki_pid;
	return (i);
}

/*
 * ICK (all for getopt), would rather hide the ugliness
 * here than taint the main code.
 *
 *  ps foo -> ps -foo
 *  ps 34 -> ps -p34
 *
 * The old convention that 't' with no trailing tty arg means the users
 * tty, is only supported if argv[1] doesn't begin with a '-'.  This same
 * feature is available with the option 'T', which takes no argument.
 */
static char *
kludge_oldps_options(char *s)
{
	int have_fmt;
	size_t len;
	char *newopts, *ns, *cp;

	/*
	 * If we have an 'o' option, then note it, since we don't want to do
	 * some types of munging.
	 */
	have_fmt = index(s, 'o') != NULL;

	len = strlen(s);
	if ((newopts = ns = malloc(len + 2)) == NULL)
		errx(1, "malloc failed");
	/*
	 * options begin with '-'
	 */
	if (*s != '-')
		*ns++ = '-';	/* add option flag */
	/*
	 * gaze to end of argv[1]
	 */
	cp = s + len - 1;
	/*
	 * if last letter is a 't' flag with no argument (in the context
	 * of the oldps options -- option string NOT starting with a '-' --
	 * then convert to 'T' (meaning *this* terminal, i.e. ttyname(0)).
	 *
	 * However, if a flag accepting a string argument is found in the
	 * option string, the remainder of the string is the argument to
	 * that flag; do not modify that argument.
	 */
	if (strcspn(s, "MNOoU") == len && *cp == 't' && *s != '-')
		*cp = 'T';
	else {
		/*
		 * otherwise check for trailing number, which *may* be a
		 * pid.
		 */
		while (cp >= s && isdigit(*cp))
			--cp;
	}
	cp++;
	memmove(ns, s, (size_t)(cp - s));	/* copy up to trailing number */
	ns += cp - s;
	/*
	 * if there's a trailing number, and not a preceding 'p' (pid) or
	 * 't' (tty) flag, then assume it's a pid and insert a 'p' flag.
	 */
	if (isdigit(*cp) &&
	    (cp == s || (cp[-1] != 't' && cp[-1] != 'p')) &&
	    (cp - 1 == s || cp[-2] != 't') && !have_fmt)
		*ns++ = 'p';
	(void)strcpy(ns, cp);		/* and append the number */

	return (newopts);
}

static void
usage(void)
{

	(void)fprintf(stderr, "%s\n%s\n%s\n",
	    "usage: ps [-aCHhjlmrSTuvwxZ] [-O|o fmt] [-p pid] [-t tty] [-U user]",
	    "          [-M core] [-N system]",
	    "       ps [-L]");
	exit(1);
}
