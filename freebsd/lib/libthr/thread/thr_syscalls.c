/*
 * Copyright (c) 2000 Jason Evans <jasone@freebsd.org>.
 * Copyright (c) 2002 Daniel M. Eischen <deischen@freebsd.org>
 * Copyright (c) 2003 Jeff Roberson <jeff@freebsd.org>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * Copyright (c) 1995-1998 John Birrell <jb@cimlogic.com.au>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by John Birrell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BIRRELL AND CONTRIBUTORS ``AS IS'' AND
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
 */

#include <sys/cdefs.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <aio.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "thr_private.h"

extern int __creat(const char *, mode_t);
extern int __sleep(unsigned int);
extern int __sys_nanosleep(const struct timespec *, struct timespec *);
extern int __sys_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
extern int __system(const char *);
extern int __tcdrain(int);
extern pid_t __wait(int *);
extern pid_t _wait4(pid_t, int *, int, struct rusage *);
extern pid_t __waitpid(pid_t, int *, int);

__weak_reference(_aio_suspend, aio_suspend);

int
_aio_suspend(const struct aiocb * const iocbs[], int niocb, const struct
    timespec *timeout)
{
	int	ret;

	_thread_enter_cancellation_point();
	ret = __sys_aio_suspend(iocbs, niocb, timeout);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__close, close);

int
__close(int fd)
{
	int	ret;

	_thread_enter_cancellation_point();
	ret = __sys_close(fd);
	_thread_leave_cancellation_point();
	
	return ret;
}
__weak_reference(___creat, creat);

int
___creat(const char *path, mode_t mode)
{
	int	ret;

	_thread_enter_cancellation_point();
	ret = __creat(path, mode);
	_thread_leave_cancellation_point();
	
	return ret;
}

__weak_reference(__fcntl, fcntl);

int
__fcntl(int fd, int cmd,...)
{
	int	ret;
	va_list	ap;
	
	_thread_enter_cancellation_point();

	va_start(ap, cmd);
	switch (cmd) {
		case F_DUPFD:
		case F_SETFD:
		case F_SETFL:
			ret = __sys_fcntl(fd, cmd, va_arg(ap, int));
			break;
		case F_GETFD:
		case F_GETFL:
			ret = __sys_fcntl(fd, cmd);
			break;
		default:
			ret = __sys_fcntl(fd, cmd, va_arg(ap, void *));
	}
	va_end(ap);

	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__fsync, fsync);

int
__fsync(int fd)
{
	int	ret;

	_thread_enter_cancellation_point();
	ret = __sys_fsync(fd);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__msync, msync);

int
__msync(void *addr, size_t len, int flags)
{
	int	ret;

	_thread_enter_cancellation_point();
	ret = __sys_msync(addr, len, flags);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(_nanosleep, nanosleep);

int
_nanosleep(const struct timespec * time_to_sleep, struct timespec *
    time_remaining)
{
	int	ret;

	_thread_enter_cancellation_point();
	ret = __sys_nanosleep(time_to_sleep, time_remaining);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__open, open);

int
__open(const char *path, int flags,...)
{
	int	ret;
	int	mode = 0;
	va_list	ap;

	_thread_enter_cancellation_point();
	
	/* Check if the file is being created: */
	if (flags & O_CREAT) {
		/* Get the creation mode: */
		va_start(ap, flags);
		mode = va_arg(ap, int);
		va_end(ap);
	}
	
	ret = __sys_open(path, flags, mode);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__poll, poll);

int
__poll(struct pollfd *fds, unsigned int nfds, int timeout)
{
	int ret;

	_thread_enter_cancellation_point();
	ret = __sys_poll(fds, nfds, timeout);
	_thread_leave_cancellation_point();

	return ret;
}

extern int __pselect(int count, fd_set *rfds, fd_set *wfds, fd_set *efds, 
		const struct timespec *timo, const sigset_t *mask);

int 
pselect(int count, fd_set *rfds, fd_set *wfds, fd_set *efds, 
	const struct timespec *timo, const sigset_t *mask)
{
	int ret;

	_thread_enter_cancellation_point();
	ret = __pselect(count, rfds, wfds, efds, timo, mask);
	_thread_leave_cancellation_point();

	return (ret);
}

__weak_reference(__read, read);

ssize_t
__read(int fd, void *buf, size_t nbytes)
{
	ssize_t	ret;

	_thread_enter_cancellation_point();
	ret = __sys_read(fd, buf, nbytes);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__readv, readv);

ssize_t
__readv(int fd, const struct iovec *iov, int iovcnt)
{
	ssize_t ret;

	_thread_enter_cancellation_point();
	ret = __sys_readv(fd, iov, iovcnt);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__select, select);

int 
__select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	struct timeval *timeout)
{
	int ret;

	_thread_enter_cancellation_point();
	ret = __sys_select(numfds, readfds, writefds, exceptfds, timeout);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(_sleep, sleep);

unsigned int
_sleep(unsigned int seconds)
{
	unsigned int	ret;

	_thread_enter_cancellation_point();
	ret = __sleep(seconds);
	_thread_leave_cancellation_point();
	
	return ret;
}

__weak_reference(_system, system);

int
_system(const char *string)
{
	int	ret;

	_thread_enter_cancellation_point();
	ret = __system(string);
	_thread_leave_cancellation_point();
	
	return ret;
}


__weak_reference(_tcdrain, tcdrain);

int
_tcdrain(int fd)
{
	int	ret;
	
	_thread_enter_cancellation_point();
	ret = __tcdrain(fd);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(_wait, wait);

pid_t
_wait(int *istat)
{
	pid_t	ret;

	_thread_enter_cancellation_point();
	ret = __wait(istat);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__wait4, wait4);

pid_t
__wait4(pid_t pid, int *istat, int options, struct rusage *rusage)
{
	pid_t ret;

	_thread_enter_cancellation_point();
	ret = _wait4(pid, istat, options, rusage);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(_waitpid, waitpid);

pid_t
_waitpid(pid_t wpid, int *status, int options)
{
	pid_t	ret;

	_thread_enter_cancellation_point();
	ret = __waitpid(wpid, status, options);
	_thread_leave_cancellation_point();
	
	return ret;
}

__weak_reference(__write, write);

ssize_t
__write(int fd, const void *buf, size_t nbytes)
{
	ssize_t	ret;

	_thread_enter_cancellation_point();
	ret = __sys_write(fd, buf, nbytes);
	_thread_leave_cancellation_point();

	return ret;
}

__weak_reference(__writev, writev);

ssize_t
__writev(int fd, const struct iovec *iov, int iovcnt)
{
	ssize_t ret;

	_thread_enter_cancellation_point();
	ret = __sys_writev(fd, iov, iovcnt);
	_thread_leave_cancellation_point();

	return ret;
}
