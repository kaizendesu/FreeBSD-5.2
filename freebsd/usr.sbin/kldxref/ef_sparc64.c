/*-
 * Copyright (c) 2003 Jake Burkholder.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/types.h>
#include <machine/elf.h>

#include <err.h>
#include <string.h>

#include "ef.h"

/*
 * Apply relocations to the values we got from the file.
 */
int
ef_reloc(elf_file_t ef, Elf_Off offset, size_t len, void *dest)
{
	const Elf_Rela *a;
	Elf_Word w;

	for (a = ef->ef_rela; a < &ef->ef_rela[ef->ef_relasz]; a++) {
		if (a->r_offset >= offset && a->r_offset < offset + len) {
			switch (ELF_R_TYPE(a->r_info)) {
			case R_SPARC_RELATIVE:
				/* load address is 0 */
				w = a->r_addend;
				memcpy((u_char *)dest + (a->r_offset - offset),
				    &w, sizeof(w));
				break;
			default:
				warnx("unhandled relocation type %u",
				    ELF_R_TYPE(a->r_info));
				break;
			}
		}
	}
	return (0);
}
