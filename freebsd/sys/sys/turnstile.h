/*
 * Copyright (c) 2002 John Baldwin <jhb@FreeBSD.org>
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
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#ifndef _SYS_TURNSTILE_H_
#define _SYS_TURNSTILE_H_

/*
 * Turnstile interface.  Non-sleepable locks use a turnstile for the
 * queue of threads blocked on them when they are contested.
 *
 * A thread calls turnstile_lookup() to look up the proper turnstile in
 * the hash table.  This function returns a pointer to the turnstile and
 * locks the associated turnstile chain.  A thread calls turnstile_wait()
 * when the lock is contested to be put on the queue and block.  If a
 * thread needs to retry a lock operation instead of blocking, it should
 * call turnstile_release() to unlock the associated turnstile chain lock.
 *
 * When a lock is released, either turnstile_signal() or turnstile_wakeup()
 * is called to mark blocked threads for a pending wakeup.
 * turnstile_signal() marks the highest priority blocked thread while
 * turnstile_wakeup() marks all blocked threads.  The turnstile_signal()
 * function returns true if the turnstile became empty as a result.  After
 * the higher level code finishes releasing the lock, turnstile_unpend()
 * must be called to wakeup the pending thread(s).
 *
 * When a lock is acquired that already has at least one thread contested
 * on it, the new owner of the lock must claim ownership of the turnstile
 * via turnstile_claim().
 *
 * Each thread allocates a turnstile at thread creation time via
 * turnstile_alloc() and frees it at thread destroy time via
 * turnstile_free().
 *
 * A function can query a turnstile to see if it is empty via
 * turnstile_empty().  The highest priority thread blocked on a turnstile
 * can be obtained via turnstile_head().
 */

struct lock_object;
struct thread;
struct turnstile;

#ifdef _KERNEL

void	init_turnstiles(void);
struct turnstile *turnstile_alloc(void);
void	turnstile_claim(struct turnstile *);
void	turnstile_free(struct turnstile *);
struct turnstile *turnstile_lookup(struct lock_object *);
void	turnstile_release(struct lock_object *);
int	turnstile_signal(struct turnstile *);
void	turnstile_unpend(struct turnstile *);
void	turnstile_wait(struct turnstile *, struct lock_object *,
	    struct thread *);
void	turnstile_wakeup(struct turnstile *);
struct thread *turnstile_head(struct turnstile *);
int	turnstile_empty(struct turnstile *);

#endif	/* _KERNEL */
#endif	/* _SYS_TURNSTILE_H_ */
