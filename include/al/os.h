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


#ifndef __ABSTRACTION_LAYER_OS_H__
#define __ABSTRACTION_LAYER_OS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>


#ifndef HAVE_PIPE2
int	pipe2(int fildes[2], int flags);
#endif


#ifndef SOCK_NONBLOCK
#	define SOCK_NONBLOCK	0x20000000
#	ifndef HAVE_ACCEPT4
		int accept4(int, struct sockaddr *, socklen_t *, int);
#	endif
#else
#	ifndef HAVE_SOCK_NONBLOCK
#		define HAVE_SOCK_NONBLOCK	1
#	endif
#endif


#ifndef HAVE_STRLCPY
static inline size_t
strlcpy(char * restrict dst, const char * restrict src, size_t size) {
	size_t src_size, cp_size;

	if (NULL == dst || NULL == src || 0 == size)
		return (0);

	src_size = strlen(src);
	cp_size = ((src_size < size) ? src_size : (size - 1));
	memcpy(dst, src, cp_size);
	dst[cp_size] = 0x00;

	return (src_size);
}
#endif


#endif /* __ABSTRACTION_LAYER_OS_H__ */
