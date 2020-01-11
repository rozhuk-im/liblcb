/*-
 * Copyright (c) 2011 - 2018 Rozhuk Ivan <rozhuk.im@gmail.com>
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
 *
 * Author: Rozhuk Ivan <rozhuk.im@gmail.com>
 *
 */


#include <sys/param.h>
#include <sys/types.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h> /* open, fcntl */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strnlen, strerror... */
#include <unistd.h> /* close, write, sysconf */

#include "al/os.h"


#ifndef HAVE_PIPE2
int
pipe2(int fildes[2], int flags) {
	int error;

	error = pipe(fildes);
	if (0 != error)
		return (error);
	if (0 != (O_NONBLOCK & flags)) {
		if (-1 == fcntl((int)fildes[0], F_SETFL, O_NONBLOCK)) {
			error = errno;
			goto err_out;
		}
		if (-1 == fcntl((int)fildes[1], F_SETFL, O_NONBLOCK)) {
			error = errno;
			goto err_out;
		}
	}

	return (0);

err_out:
	close(fildes[0]);
	close(fildes[1]);
	fildes[0] = -1;
	fildes[1] = -1;

	return (error);
}
#endif


#ifndef HAVE_ACCEPT4
int
accept4(int skt, struct sockaddr *addr, socklen_t *addrlen, int flags) {
	int s;

	s = accept(skt, addr, addrlen);
	if (-1 == s)
		return (-1);
	if (0 != (SOCK_NONBLOCK & flags)) {
		if (-1 == fcntl((int)s, F_SETFL, O_NONBLOCK)) {
			close(s);
			return (-1);
		}
	}

	return (s);
}
#endif
