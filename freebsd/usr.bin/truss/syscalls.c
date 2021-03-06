/*
 * Copryight 1997 Sean Eric Fagan
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
 *	This product includes software developed by Sean Eric Fagan
 * 4. Neither the name of the author may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
 */

#ifndef lint
static const char rcsid[] =
  "$FreeBSD$";
#endif /* not lint */

/*
 * This file has routines used to print out system calls and their
 * arguments.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "truss.h"
#include "extern.h"
#include "syscall.h"

/*
 * This should probably be in its own file.
 */

struct syscall syscalls[] = {
	{ "readlink", 1, 3,
	  { { String, 0 } , { String | OUT, 1 }, { Int, 2 }}},
	{ "lseek", 2, 3,
	  { { Int, 0 }, {Quad, 2 }, { Int, 4 }}},
	{ "mmap", 2, 6,
	  { { Hex, 0 }, {Int, 1}, {Hex, 2}, {Hex, 3}, {Int, 4}, {Quad, 6}}},
	{ "open", 1, 3,
	  { { String | IN, 0} , { Hex, 1}, {Octal, 2}}},
	{ "linux_open", 1, 3,
	  { { String, 0 }, { Hex, 1}, { Octal, 2 }}},
	{ "close", 1, 1,
	  { { Int, 0 } } },
	{ "link", 0, 2,
	  { { String, 0 }, { String, 1 }}},
	{ "unlink", 0, 1,
	  { { String, 0 }}},
	{ "chdir", 0, 1,
	  { { String, 0 }}},
	{ "mknod", 0, 3,
	  { { String, 0 }, { Octal, 1 }, { Int, 3 }}},
	{ "chmod", 0, 2,
	  { { String, 0 }, { Octal, 1 }}},
	{ "chown", 0, 3,
	  { { String, 0 }, { Int, 1 }, { Int, 2 }}},
	{ "mount", 0, 4,
	  { { String, 0 }, { String, 1 }, { Int, 2 }, { Ptr, 3 }}},
	{ "umount", 0, 2,
	  { { String, 0 }, { Int, 2 }}},
	{ "fstat", 1, 2,
	  { { Int, 0},  {Ptr | OUT , 1 }}},
	{ "stat", 1, 2,
	  { { String | IN, 0 }, { Ptr | OUT, 1 }}},
	{ "lstat", 1, 2,
	  { { String | IN, 0 }, { Ptr | OUT, 1 }}},
	{ "linux_newstat", 1, 2,
	  { { String | IN, 0 }, { Ptr | OUT, 1 }}},
	{ "linux_newfstat", 1, 2,
	  { { Int, 0 }, { Ptr | OUT, 1 }}},
	{ "write", 1, 3,
	  { { Int, 0 }, { Ptr | IN, 1 }, { Int, 2 }}},
	{ "ioctl", 1, 3,
	  { { Int, 0 }, { Ioctl, 1 }, { Hex, 2 }}},
	{ "break", 1, 1, { { Hex, 0 }}},
	{ "exit", 0, 1, { { Hex, 0 }}},
	{ "access", 1, 2, { { String | IN, 0 }, { Int, 1 }}},
	{ "sigaction", 1, 3,
	  { { Signal, 0 }, { Ptr | IN, 1 }, { Ptr | OUT, 2 }}},
	{ "accept", 1, 3,
	  { { Hex, 0 }, { Sockaddr | OUT, 1 }, { Ptr | OUT, 2 } } },
	{ "bind", 1, 3,
	  { { Hex, 0 }, { Sockaddr | IN, 1 }, { Int, 2 } } },
	{ "connect", 1, 3,
	  { { Hex, 0 }, { Sockaddr | IN, 1 }, { Int, 2 } } },
	{ "getpeername", 1, 3,
	  { { Hex, 0 }, { Sockaddr | OUT, 1 }, { Ptr | OUT, 2 } } },
	{ "getsockname", 1, 3,
	  { { Hex, 0 }, { Sockaddr | OUT, 1 }, { Ptr | OUT, 2 } } },
	{ "recvfrom", 1, 6,
	  { { Hex, 0 }, { Ptr | IN, 1 }, { Int, 3 }, { Hex, 3 }, { Sockaddr | OUT, 4 }, { Ptr | OUT, 5 } } },
	{ "sendto", 1, 6,
	  { { Hex, 0 }, { Ptr | IN, 1 }, { Int, 3 }, { Hex, 3 }, { Sockaddr | IN, 4 }, { Ptr | IN, 5 } } },
	{ "execve", 1, 3,
	  { { String | IN, 0 }, { StringArray | IN, 1 }, { StringArray | IN, 2 } } },
	{ "linux_execve", 1, 3,
	  { { String | IN, 0 }, { StringArray | IN, 1 }, { StringArray | IN, 2 } } },
	{ "kldload", 0, 1, { { String | IN, 0 }}},
	{ "kldunload", 0, 1, { { Int, 0 }}},
	{ "kldfind", 0, 1, { { String | IN, 0 }}},
	{ "kldnext", 0, 1, { { Int, 0 }}},
	{ "kldstat", 0, 2, { { Int, 0 }, { Ptr, 1 }}},
	{ "kldfirstmod", 0, 1, { { Int, 0 }}},
	{ 0, 0, 0, { { 0, 0 }}},
};

/*
 * If/when the list gets big, it might be desirable to do it
 * as a hash table or binary search.
 */

struct syscall *
get_syscall(const char *name) {
	struct syscall *sc = syscalls;

	while (sc->name) {
		if (!strcmp(name, sc->name))
			return sc;
		sc++;
	}
	return NULL;
}

/*
 * get_struct
 *
 * Copy a fixed amount of bytes from the process.
 */

static int
get_struct(int procfd, void *offset, void *buf, int len) {
	char *pos;
	FILE *p;
	int c, fd;

	if ((fd = dup(procfd)) == -1)
		err(1, "dup");
	if ((p = fdopen(fd, "r")) == NULL)
		err(1, "fdopen");
	if (fseeko(p, (uintptr_t)offset, SEEK_SET) == 0) {
		for (pos = (char *)buf; len--; pos++) {
			if ((c = fgetc(p)) == EOF)
				return (-1);
			*pos = c;
		}
	} else
		bzero(buf, len);
	fclose(p);
	return 0;
}

/*
 * get_string
 * Copy a string from the process.  Note that it is
 * expected to be a C string, but if max is set, it will
 * only get that much.
 */

char *
get_string(int procfd, void *offset, int max) {
	char *buf;
	int size, len, c, fd;
	FILE *p;

	if ((fd = dup(procfd)) == -1)
		err(1, "dup");
	if ((p = fdopen(fd, "r")) == NULL)
		err(1, "fdopen");
	buf = malloc( size = (max ? max : 64 ) );
	len = 0;
	buf[0] = 0;
	if (fseeko(p, (uintptr_t)offset, SEEK_SET) == 0) {
		while ((c = fgetc(p)) != EOF) {
			buf[len++] = c;
			if (c == 0 || len == max) {
				buf[len] = 0;
				break;
			}
			if (len == size) {
				char *tmp;
				tmp = realloc(buf, size+64);
				if (tmp == NULL) {
					buf[len] = 0;
					break;
				}
				size += 64;
				buf = tmp;
			}
		}
	}
	fclose(p);
	return (buf);
}


/*
 * Gag.  This is really unportable.  Multiplication is more portable.
 * But slower, from the code I saw.
 */

static long long
make_quad(unsigned long p1, unsigned long p2) {
  union {
    long long ll;
    unsigned long l[2];
  } t;
  t.l[0] = p1;
  t.l[1] = p2;
  return t.ll;
}


/*
 * print_arg
 * Converts a syscall argument into a string.  Said string is
 * allocated via malloc(), so needs to be free()'d.  The file
 * descriptor is for the process' memory (via /proc), and is used
 * to get any data (where the argument is a pointer).  sc is
 * a pointer to the syscall description (see above); args is
 * an array of all of the system call arguments.
 */

char *
print_arg(int fd, struct syscall_args *sc, unsigned long *args) {
  char *tmp = NULL;
  switch (sc->type & ARG_MASK) {
  case Hex:
    asprintf(&tmp, "0x%lx", args[sc->offset]);
    break;
  case Octal:
    asprintf(&tmp, "0%lo", args[sc->offset]);
    break;
  case Int:
    asprintf(&tmp, "%ld", args[sc->offset]);
    break;
  case String:
    {
      char *tmp2;
      tmp2 = get_string(fd, (void*)args[sc->offset], 0);
      asprintf(&tmp, "\"%s\"", tmp2);
      free(tmp2);
    }
  break;
  case StringArray:
    {
      int num, size, i;
      char *tmp2;
      char *string;
      char *strarray[100];	/* XXX This is ugly. */

      if (get_struct(fd, (void *)args[sc->offset], (void *)&strarray,
                     sizeof(strarray)) == -1) {
	err(1, "get_struct %p", (void *)args[sc->offset]);
      }
      num = 0;
      size = 0;

      /* Find out how large of a buffer we'll need. */
      while (strarray[num] != NULL) {
	string = get_string(fd, (void*)strarray[num], 0);
        size += strlen(string);
	free(string);
	num++;
      }
      size += 4 + (num * 4);
      tmp = (char *)malloc(size);
      tmp2 = tmp;

      tmp2 += sprintf(tmp2, " [");
      for (i = 0; i < num; i++) {
	string = get_string(fd, (void*)strarray[i], 0);
        tmp2 += sprintf(tmp2, " \"%s\"%c", string, (i+1 == num) ? ' ' : ',');
	free(string);
      }
      tmp2 += sprintf(tmp2, "]");
    }
  break;
  case Quad:
    {
      unsigned long long t;
      unsigned long l1, l2;
      l1 = args[sc->offset];
      l2 = args[sc->offset+1];
      t = make_quad(l1, l2);
      asprintf(&tmp, "0x%qx", t);
      break;
    }
  case Ptr:
    asprintf(&tmp, "0x%lx", args[sc->offset]);
    break;
  case Ioctl:
    {
      const char *temp = ioctlname(args[sc->offset]);
      if (temp)
	tmp = strdup(temp);
      else
	asprintf(&tmp, "0x%lx", args[sc->offset]);
    }
    break;
  case Signal:
    {
      long sig;

      sig = args[sc->offset];
      if (sig > 0 && sig < NSIG) {
	int i;
	asprintf(&tmp, "sig%s", sys_signame[sig]);
	for (i = 0; tmp[i] != '\0'; ++i)
	  tmp[i] = toupper(tmp[i]);
      } else
        asprintf(&tmp, "%ld", sig);
    }
    break;
  case Sockaddr:
    {
      struct sockaddr_storage ss;
      char addr[64];
      struct sockaddr_in *lsin;
      struct sockaddr_in6 *lsin6;
      struct sockaddr_un *sun;
      struct sockaddr *sa;
      char *p;
      u_char *q;
      int i;

      if (args[sc->offset] == 0) {
	      asprintf(&tmp, "NULL");
	      break;
      }

      /* yuck: get ss_len */
      if (get_struct(fd, (void *)args[sc->offset], (void *)&ss,
	sizeof(ss.ss_len) + sizeof(ss.ss_family)) == -1)
	err(1, "get_struct %p", (void *)args[sc->offset]);
      /* sockaddr_un never have the length filled in! */
      if (ss.ss_family == AF_UNIX) {
	if (get_struct(fd, (void *)args[sc->offset], (void *)&ss,
	  sizeof(*sun))
	  == -1)
	  err(2, "get_struct %p", (void *)args[sc->offset]);
      } else {
	if (get_struct(fd, (void *)args[sc->offset], (void *)&ss, ss.ss_len)
	  == -1)
	  err(2, "get_struct %p", (void *)args[sc->offset]);
      }

      switch (ss.ss_family) {
      case AF_INET:
	lsin = (struct sockaddr_in *)&ss;
	inet_ntop(AF_INET, &lsin->sin_addr, addr, sizeof addr);
	asprintf(&tmp, "{ AF_INET %s:%d }", addr, htons(lsin->sin_port));
	break;
      case AF_INET6:
	lsin6 = (struct sockaddr_in6 *)&ss;
	inet_ntop(AF_INET6, &lsin6->sin6_addr, addr, sizeof addr);
	asprintf(&tmp, "{ AF_INET6 [%s]:%d }", addr, htons(lsin6->sin6_port));
	break;
      case AF_UNIX:
        sun = (struct sockaddr_un *)&ss;
        asprintf(&tmp, "{ AF_UNIX \"%s\" }", sun->sun_path);
	break;
      default:
	sa = (struct sockaddr *)&ss;
        asprintf(&tmp, "{ sa_len = %d, sa_family = %d, sa_data = {%n%*s } }",
	  (int)sa->sa_len, (int)sa->sa_family, &i,
	  6 * (int)(sa->sa_len - ((char *)&sa->sa_data - (char *)sa)), "");
	if (tmp != NULL) {
	  p = tmp + i;
          for (q = (u_char *)&sa->sa_data; q < (u_char *)sa + sa->sa_len; q++)
            p += sprintf(p, " %#02x,", *q);
	}
      }
    }
    break;
  }
  return tmp;
}

#define timespecsubt(tvp, uvp, vvp)					\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_nsec = (tvp)->tv_nsec - (uvp)->tv_nsec;	\
		if ((vvp)->tv_nsec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_nsec += 1000000000;			\
		}							\
	} while (0)

/*
 * print_syscall
 * Print (to outfile) the system call and its arguments.  Note that
 * nargs is the number of arguments (not the number of words; this is
 * potentially confusing, I know).
 */

void
print_syscall(struct trussinfo *trussinfo, const char *name, int nargs, char **s_args) {
  int i;
  int len = 0;
  struct timespec timediff;

  if (trussinfo->flags & FOLLOWFORKS)
    len += fprintf(trussinfo->outfile, "%5d: ", trussinfo->pid);

  if (name != NULL && (!strcmp(name, "execve") || !strcmp(name, "exit"))) {
    clock_gettime(CLOCK_REALTIME, &trussinfo->after);
  }

  if (trussinfo->flags & ABSOLUTETIMESTAMPS) {
    timespecsubt(&trussinfo->after, &trussinfo->start_time, &timediff);
    len += fprintf(trussinfo->outfile, "%ld.%09ld ",
		   (long)timediff.tv_sec, timediff.tv_nsec);
  }

  if (trussinfo->flags & RELATIVETIMESTAMPS) {
    timespecsubt(&trussinfo->after, &trussinfo->before, &timediff);
    len += fprintf(trussinfo->outfile, "%ld.%09ld ",
		   (long)timediff.tv_sec, timediff.tv_nsec);
  }

  len += fprintf(trussinfo->outfile, "%s(", name);

  for (i = 0; i < nargs; i++) {
    if (s_args[i])
      len += fprintf(trussinfo->outfile, "%s", s_args[i]);
    else
      len += fprintf(trussinfo->outfile, "<missing argument>");
    len += fprintf(trussinfo->outfile, "%s", i < (nargs - 1) ? "," : "");
  }
  len += fprintf(trussinfo->outfile, ")");
  for (i = 0; i < 6 - (len / 8); i++)
	fprintf(trussinfo->outfile, "\t");
}

void
print_syscall_ret(struct trussinfo *trussinfo, const char *name, int nargs,
    char **s_args, int errorp, long retval)
{
  print_syscall(trussinfo, name, nargs, s_args);
  if (errorp) {
    fprintf(trussinfo->outfile, " ERR#%d '%s'\n", retval, strerror(retval));
  } else {
    fprintf(trussinfo->outfile, " = %ld (0x%lx)\n", retval, retval);
  }
}
