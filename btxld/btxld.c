/*
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
  "$FreeBSD$";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "btx.h"
#include "elfh.h"
#include "endian.h"
#include "i386_a.out.h"

#define BTX_PATH		"/sys/boot/i386/btx"

#define I_LDR	0		/* BTX loader */
#define I_BTX	1		/* BTX kernel */
#define I_CLNT	2		/* Client program */

#define F_BIN	0		/* Binary */
#define F_AOUT	1		/* ZMAGIC a.out */
#define F_ELF	2		/* 32-bit ELF */
#define F_CNT	3		/* Number of formats */

#define IMPURE	1		/* Writable text */
#define MAXU32	0xffffffff	/* Maximum unsigned 32-bit quantity */

#define align(x, y) (((x) + (y) - 1) & ~((y) - 1))

struct hdr {
    uint32_t fmt;		/* Format */
    uint32_t flags;		/* Bit flags */
    uint32_t size;		/* Size of file */
    uint32_t text;		/* Size of text segment */
    uint32_t data;		/* Size of data segment */
    uint32_t bss;		/* Size of bss segment */
    uint32_t org;		/* Program origin */
    uint32_t entry;		/* Program entry point */
};

static const char *const fmtlist[] = {"bin", "aout", "elf"};

static const char binfo[] =
    "kernel: ver=%u.%02u size=%x load=%x entry=%x map=%uM "
    "pgctl=%x:%x\n";
static const char cinfo[] =
    "client: fmt=%s size=%x text=%x data=%x bss=%x entry=%x\n";
static const char oinfo[] =
    "output: fmt=%s size=%x text=%x data=%x org=%x entry=%x\n";

static const char *lname =
    BTX_PATH "/btxldr/btxldr";	/* BTX loader */
static const char *bname =
    BTX_PATH "/btx/btx";	/* BTX kernel */
static const char *oname =
    "a.out";			/* Output filename */

static int ppage = -1;		/* First page present */
static int wpage = -1;		/* First page writable */

static unsigned int format; 	/* Output format */

static uint32_t centry; 	/* Client entry address */
static uint32_t lentry; 	/* Loader entry address */

static int Eflag;		/* Client entry option */

static int quiet;		/* Inhibit warnings */
static int verbose;		/* Display information */

static const char *tname;	/* Temporary output file */
static const char *fname;	/* Current input file */

static void cleanup(void);
static void btxld(const char *);
static void getbtx(int, struct btx_hdr *);
static void gethdr(int, struct hdr *);
static void puthdr(int, struct hdr *);
static void copy(int, int, size_t, off_t);
static size_t readx(int, void *, size_t, off_t);
static void writex(int, const void *, size_t);
static void seekx(int, off_t);
static unsigned int optfmt(const char *);
static uint32_t optaddr(const char *);
static int optpage(const char *, int);
static void Warn(const char *, const char *, ...);
static void usage(void);

/*
 * A link editor for BTX clients.
 */
int
main(int argc, char *argv[])
{
    int c;

    while ((c = getopt(argc, argv, "qvb:E:e:f:l:o:P:W:")) != -1)
		switch (c) {
		case 'q':
	    	quiet = 1;
	    	break;
		case 'v':
		    verbose = 1;
		    break;
		case 'b':
		    bname = optarg;				/* sets the btx kernel */
		    break;
		case 'E':
		    centry = optaddr(optarg);
		    Eflag = 1;
		    break;
		case 'e':
		    lentry = optaddr(optarg);	/* sets entry point as an unsigned long */
		    break;
		case 'f':
		    format = optfmt(optarg);	/* sets format = "aout" */
		    break;
		case 'l':
		    lname = optarg;				/* sets loader name */
		    break;
		case 'o':
		    oname = optarg;				/* sets name of output file */
		    break;
		case 'P':
		    ppage = optpage(optarg, 1);
		    break;
		case 'W':
		    wpage = optpage(optarg, BTX_MAXCWR);
		    break;
		default:
		    usage();
		}
    argc -= optind;
    argv += optind;
    if (argc != 1)
		usage();
    atexit(cleanup);
    btxld(*argv);
    return 0;
}

/*
 * Clean up after errors.
 */
static void
cleanup(void)
{
    if (tname)
	remove(tname);
}

/*
 * Read the input files; write the output file; display information.
 */
static void
btxld(const char *iname)
{
    char name[FILENAME_MAX];
    struct btx_hdr btx, btxle;
    struct hdr ihdr, ohdr;
    unsigned int ldr_size, cwr;
    int fdi[3], fdo, i;

    ldr_size = 0;

    for (i = I_LDR; i <= I_CLNT; i++) {		/* i = 0; i <= 2; i++ */
		fname = (i == I_LDR ? lname : i == I_BTX ? bname : iname);
		if ((fdi[i] = open(fname, O_RDONLY)) == -1)
	    	err(2, "%s", fname);
		switch (i) {
		case I_LDR:		/* Read /boot/loader header into ihdr */
		    gethdr(fdi[i], &ihdr);
			if (ihdr.fmt != F_BIN)
				Warn(fname, "Loader format is %s; processing as %s",
				fmtlist[ihdr.fmt], fmtlist[F_BIN]);
	    		ldr_size = ihdr.size;	/* Store loader's size */
			break;
		case I_BTX:		/* Set paging control, text size, and entry point for btx kernel*/
		    getbtx(fdi[i], &btx);
		    break;
		case I_CLNT:	/* Read client's header into &ihdr */
		    gethdr(fdi[i], &ihdr);
		    if (ihdr.org && ihdr.org != BTX_PGSIZE)
				Warn(fname, "Client origin is 0x%x; expecting 0 or 0x%x",
				ihdr.org, BTX_PGSIZE);
		}
    }
    memset(&ohdr, 0, sizeof(ohdr));
    ohdr.fmt = format;			/* ohdr.fmt = "aout" */
    ohdr.text = ldr_size;		/* ohdr.text = sizeof(/boot/loader) */

    ohdr.data = btx.btx_textsz + ihdr.size;		/* size of btx kern and client text */
    ohdr.org = lentry;			/* client load address 0x200000 */
    ohdr.entry = lentry;		/* client entry point 0x200000 */
    cwr = 0;

	/* wpage = first page writable ? */
    if (wpage > 0 || (wpage == -1 && !(ihdr.flags & IMPURE))) {
	if (wpage > 0)
	    cwr = wpage;
	else {
	    cwr = howmany(ihdr.text, BTX_PGSIZE);	/* # of client text pages */
	    if (cwr > BTX_MAXCWR)
		cwr = BTX_MAXCWR;
	}
    }

	/* ppage = first page present ? */
    if (ppage > 0 || (ppage && wpage && ihdr.org >= BTX_PGSIZE)) {
	btx.btx_flags |= BTX_MAPONE;
	if (!cwr)
	    cwr++;
    }
    btx.btx_pgctl -= cwr;
    btx.btx_entry = Eflag ? centry : ihdr.entry;	/* use ihdr.entry for client entry */
    if (snprintf(name, sizeof(name), "%s.tmp", oname) >= sizeof(name))
	errx(2, "%s: Filename too long", oname);
    if ((fdo = open(name, O_CREAT | O_TRUNC | O_WRONLY, 0666)) == -1)
	err(2, "%s", name);
    if (!(tname = strdup(name)))
	err(2, NULL);
    puthdr(fdo, &ohdr);
    for (i = I_LDR; i <= I_CLNT; i++) {	/* i = 0; i <= 2; i++ */
	fname = i == I_LDR ? lname : i == I_BTX ? bname : iname;
	switch (i) {
	case I_LDR:
	    copy(fdi[i], fdo, ldr_size, 0);
	    seekx(fdo, ohdr.size += ohdr.text);
	    break;
	case I_BTX:
	    btxle = btx;
	    btxle.btx_pgctl = HTOLE16(btxle.btx_pgctl);
	    btxle.btx_textsz = HTOLE16(btxle.btx_textsz);
	    btxle.btx_entry = HTOLE32(btxle.btx_entry);
	    writex(fdo, &btxle, sizeof(btxle));
	    copy(fdi[i], fdo, btx.btx_textsz - sizeof(btx),
		 sizeof(btx));
	    break;
	case I_CLNT:
	    copy(fdi[i], fdo, ihdr.size, 0);
	    if (ftruncate(fdo, ohdr.size += ohdr.data))
		err(2, "%s", tname);
	}
	if (close(fdi[i]))
	    err(2, "%s", fname);
    }
    if (close(fdo))
	err(2, "%s", tname);
    if (rename(tname, oname))
	err(2, "%s: Can't rename to %s", tname, oname);
    tname = NULL;
    if (verbose) {
	printf(binfo, btx.btx_majver, btx.btx_minver, btx.btx_textsz,
	       BTX_ORIGIN(btx), BTX_ENTRY(btx), BTX_MAPPED(btx) *
	       BTX_PGSIZE / 0x100000, !!(btx.btx_flags & BTX_MAPONE),
	       BTX_MAPPED(btx) - btx.btx_pgctl - BTX_PGBASE /
	       BTX_PGSIZE - BTX_MAPPED(btx) * 4 / BTX_PGSIZE);
	printf(cinfo, fmtlist[ihdr.fmt], ihdr.size, ihdr.text,
	       ihdr.data, ihdr.bss, ihdr.entry);
	printf(oinfo, fmtlist[ohdr.fmt], ohdr.size, ohdr.text,
	       ohdr.data, ohdr.org, ohdr.entry);
    }
}

/*
 * Read BTX file header.
 */
static void
getbtx(int fd, struct btx_hdr * btx)
{
    if (readx(fd, btx, sizeof(*btx), 0) != sizeof(*btx) ||
	btx->btx_magic[0] != BTX_MAG0 ||
	btx->btx_magic[1] != BTX_MAG1 ||
	btx->btx_magic[2] != BTX_MAG2)
		errx(1, "%s: Not a BTX kernel", fname);
    btx->btx_pgctl = LE16TOH(btx->btx_pgctl);	/* paging control as uint16 */
    btx->btx_textsz = LE16TOH(btx->btx_textsz);	/* text size as uint16*/
    btx->btx_entry = LE32TOH(btx->btx_entry);	/* btx entry point as uint32 */
}

/*
 * Get file size and read a.out or ELF header.
 */
static void
gethdr(int fd, struct hdr *hdr)
{
    struct stat sb;
    const struct i386_exec *ex;
    const Elf32_Ehdr *ee;
    const Elf32_Phdr *ep;
    void *p;
    unsigned int fmt, x, n, i;

    memset(hdr, 0, sizeof(*hdr));	/* Clear loader data from ihdr */
    if (fstat(fd, &sb))
		err(2, "%s", fname);
    if (sb.st_size > MAXU32)	/* > 0xffffffff */
		errx(1, "%s: Too big", fname);
    hdr->size = sb.st_size;

	/* Create a new mapping in vaddr for the client header */ 
    if ((p = mmap(NULL, hdr->size, PROT_READ, MAP_SHARED, fd,
		  0)) == MAP_FAILED)
		err(2, "%s", fname);
    for (fmt = F_CNT - 1; !hdr->fmt && fmt; fmt--)	/* fmt = 2; !ihdr->fmt && fmt; fmt-- */
		switch (fmt) {
			case F_AOUT:	/* fmt == 1 */
	    		ex = p;
	    		if (hdr->size >= sizeof(struct i386_exec) && !I386_N_BADMAG(*ex)) {
					hdr->fmt = fmt;
					x = I386_N_GETMAGIC(*ex);
					if (x == OMAGIC || x == NMAGIC) {
		    			if (x == NMAGIC)
							Warn(fname, "Treating %s NMAGIC as OMAGIC",
							fmtlist[fmt]);
		   			 	hdr->flags |= IMPURE;
					}
					hdr->text = LE32TOH(ex->a_text);
					hdr->data = LE32TOH(ex->a_data);
					hdr->bss = LE32TOH(ex->a_bss);
					hdr->entry = LE32TOH(ex->a_entry);
					if (LE32TOH(ex->a_entry) >= BTX_PGSIZE)
						hdr->org = BTX_PGSIZE;
	    		}
	    		break;
			case F_ELF:		/* fmt == 2 */
			    ee = p;
			    if (hdr->size >= sizeof(Elf32_Ehdr) && IS_ELF(*ee)) {
					hdr->fmt = fmt;
					for (n = i = 0; i < LE16TOH(ee->e_phnum); i++) {	/* for each program header */
					    ep = (void *)((uint8_t *)p + LE32TOH(ee->e_phoff)
							+ LE16TOH(ee->e_phentsize) * i);
				    	if (LE32TOH(ep->p_type) == PT_LOAD)		/* program header loadable? */
							switch (n++) {
								case 0:	/* client text segment */
								    hdr->text = LE32TOH(ep->p_filesz);	/* set client text size */
								    hdr->org = LE32TOH(ep->p_paddr);	/* set client load address */
								    if (LE32TOH(ep->p_flags) & PF_W)	/* writable? */
										hdr->flags |= IMPURE;
								    break;
								case 1:	/* client data segments */
								    hdr->data = LE32TOH(ep->p_filesz);	/* set client data size */
								    hdr->bss = LE32TOH(ep->p_memsz)		/* set client bss */
												- LE32TOH(ep->p_filesz);
								    break;
								case 2:	/* extra segment */
							    Warn(fname,
								"Ignoring extra %s PT_LOAD segments",
								fmtlist[fmt]);
							}
					}
					hdr->entry = LE32TOH(ee->e_entry);
	    		}
		}
    if (munmap(p, hdr->size))
	err(2, "%s", fname);
}

/*
 * Write a.out or ELF header.
 */

struct i386_exec {
	uint32_t	a_midmag;	/* flags<<26 | mid<<16 | magic */
	uint32_t	a_text;		/* text segment size */
	uint32_t	a_data;		/* initialized data size */
	uint32_t	a_bss;		/* uninitialized data size */
	uint32_t	a_syms;		/* symbol table size */
	uint32_t	a_entry;	/* entry point */
	uint32_t	a_trsize;	/* text relocation size */
	uint32_t	a_drsize;	/* data relocation size */
};

static void
puthdr(int fd, struct hdr *hdr)
{
    struct i386_exec ex;
    struct elfh eh;

    switch (hdr->fmt) {
    case F_AOUT:	/* We use this case */
	memset(&ex, 0, sizeof(ex));
	I386_N_SETMAGIC(ex, ZMAGIC, MID_ZERO, 0);	/* set ex.a_midmag */
	hdr->text = I386_N_ALIGN(ex, hdr->text);
	ex.a_text = HTOLE32(hdr->text);		/* HTOLE32 converts to uint32_t */
	hdr->data = I386_N_ALIGN(ex, hdr->data);
	ex.a_data = HTOLE32(hdr->data);
	ex.a_entry = HTOLE32(hdr->entry);
	writex(fd, &ex, sizeof(ex));
	hdr->size = I386_N_ALIGN(ex, sizeof(ex));
	seekx(fd, hdr->size);
	break;
    case F_ELF:
	eh = elfhdr;
	eh.e.e_entry = HTOLE32(hdr->entry);
	eh.p[0].p_vaddr = eh.p[0].p_paddr = HTOLE32(hdr->org);
	eh.p[0].p_filesz = eh.p[0].p_memsz = HTOLE32(hdr->text);
	eh.p[1].p_offset = HTOLE32(LE32TOH(eh.p[0].p_offset) +
	    LE32TOH(eh.p[0].p_filesz));
	eh.p[1].p_vaddr = eh.p[1].p_paddr =
	    HTOLE32(align(LE32TOH(eh.p[0].p_paddr) + LE32TOH(eh.p[0].p_memsz),
	    4));
	eh.p[1].p_filesz = eh.p[1].p_memsz = HTOLE32(hdr->data);
	eh.sh[2].sh_addr = eh.p[0].p_vaddr;
	eh.sh[2].sh_offset = eh.p[0].p_offset;
	eh.sh[2].sh_size = eh.p[0].p_filesz;
	eh.sh[3].sh_addr = eh.p[1].p_vaddr;
	eh.sh[3].sh_offset = eh.p[1].p_offset;
	eh.sh[3].sh_size = eh.p[1].p_filesz;
	writex(fd, &eh, sizeof(eh));
	hdr->size = sizeof(eh);
    }
}

/*
 * Safe copy from input file to output file.
 */
static void
copy(int fdi, int fdo, size_t nbyte, off_t offset)
{
    char buf[8192];
    size_t n;

    while (nbyte) {
	if ((n = sizeof(buf)) > nbyte)
	    n = nbyte;
	if (readx(fdi, buf, n, offset) != n)
	    errx(2, "%s: Short read", fname);
	writex(fdo, buf, n);
	nbyte -= n;
	offset = -1;
    }
}

/*
 * Safe read from input file.
 */
static size_t
readx(int fd, void *buf, size_t nbyte, off_t offset)
{
    ssize_t n;

    if (offset != -1 && lseek(fd, offset, SEEK_SET) != offset)
		err(2, "%s", fname);
    if ((n = read(fd, buf, nbyte)) == -1)
		err(2, "%s", fname);
    return n;
}

/*
 * Safe write to output file.
 */
static void
writex(int fd, const void *buf, size_t nbyte)
{
    ssize_t n;

    if ((n = write(fd, buf, nbyte)) == -1)
	err(2, "%s", tname);
    if (n != nbyte)
	errx(2, "%s: Short write", tname);
}

/*
 * Safe seek in output file.
 */
static void
seekx(int fd, off_t offset)
{
    if (lseek(fd, offset, SEEK_SET) != offset)
	err(2, "%s", tname);
}

/*
 * Convert an option argument to a format code.
 */
static unsigned int
optfmt(const char *arg)
{
    unsigned int i;

    for (i = 0; i < F_CNT && strcmp(arg, fmtlist[i]); i++);
    if (i == F_CNT)
		errx(1, "%s: Unknown format", arg);
    return i;
}

/*
 * Convert an option argument to an address.
 */
static uint32_t
optaddr(const char *arg)
{
    char *s;
    unsigned long x;

    errno = 0;
    x = strtoul(arg, &s, 0);
    if (errno || !*arg || *s || x > MAXU32)
		errx(1, "%s: Illegal address", arg);
    return x;
}

/*
 * Convert an option argument to a page number.
 */
static int
optpage(const char *arg, int hi)
{
    char *s;
    long x;

    errno = 0;
    x = strtol(arg, &s, 0);
    if (errno || !*arg || *s || x < 0 || x > hi)
	errx(1, "%s: Illegal page number", arg);
    return x;
}

/*
 * Display a warning.
 */
static void
Warn(const char *locus, const char *fmt, ...)
{
    va_list ap;
    char *s;

    if (!quiet) {
	asprintf(&s, "%s: Warning: %s", locus, fmt);
	va_start(ap, fmt);
	vwarnx(s, ap);
	va_end(ap);
	free(s);
    }
}

/*
 * Display usage information.
 */
static void
usage(void)
{
    fprintf(stderr, "%s\n%s\n",
    "usage: btxld [-qv] [-b file] [-E address] [-e address] [-f format]",
    "             [-l file] [-o filename] [-P page] [-W page] file");
    exit(1);
}
