/*-
 * Copyright (c) 2011 - 2017 Rozhuk Ivan <rozhuk.im@gmail.com>
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

#ifdef __linux__ /* Linux specific code. */
#	define _GNU_SOURCE /* See feature_test_macros(7) */
#	define __USE_GNU 1
#endif /* Linux specific code. */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h> /* snprintf, fprintf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strerror... */

#include "utils/macro.h"
#include "utils/mem_utils.h"
#include "utils/str2num.h"
#include "net/socket_address.h"


static const sa_family_t family_list[] = {
	AF_INET, AF_INET6
};


// copy sockaddr_storage struct
void
sa_copy(const void *src, void *dst) {

	if (NULL == src || NULL == dst || src == dst)
		return;

	switch (((const struct sockaddr*)src)->sa_family) {
	case AF_INET:
		memcpy(dst, src, sizeof(struct sockaddr_in));
		break;
	case AF_INET6:
		memcpy(dst, src, sizeof(struct sockaddr_in6));
		break;
	default:
		memcpy(dst, src, sizeof(struct sockaddr_storage));
		break;
	}
}

int
sa_init(struct sockaddr_storage *addr, const sa_family_t family,
    const void *sin_addr, const uint16_t port) {

	if (NULL == addr)
		return (EINVAL);

	switch (family) {
	case AF_INET:
		mem_bzero(addr, sizeof(struct sockaddr_in));
#ifdef BSD /* BSD specific code. */
		((struct sockaddr_in*)addr)->sin_len = sizeof(struct sockaddr_in);
#endif /* BSD specific code. */
		((struct sockaddr_in*)addr)->sin_family = AF_INET;
		//addr->sin_port = 0;
		//addr->sin_addr.s_addr = 0;
		break;
	case AF_INET6:
		mem_bzero(addr, sizeof(struct sockaddr_in6));
#ifdef BSD /* BSD specific code. */
		((struct sockaddr_in6*)addr)->sin6_len = sizeof(struct sockaddr_in6);
#endif /* BSD specific code. */
		((struct sockaddr_in6*)addr)->sin6_family = AF_INET6;
		//((struct sockaddr_in6*)addr)->sin6_port = 0;
		//((struct sockaddr_in6*)addr)->sin6_flowinfo = 0;
		//((struct sockaddr_in6*)addr)->sin6_addr
		//((struct sockaddr_in6*)addr)->sin6_scope_id = 0;
		break;
	default:
		return (EINVAL);
	}

	sa_addr_set(addr, sin_addr);
	if (0 == port)
		return (0);

	return (sa_port_set(addr, port));
}

sa_family_t
sa_family(const struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (0);

	switch (addr->ss_family) {
	case AF_INET:
	case AF_INET6:
		return (addr->ss_family);
	}

	return (0);
}

socklen_t
sa_size(const struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (0);

	switch (addr->ss_family) {
	case AF_INET:
		return (sizeof(struct sockaddr_in));
	case AF_INET6:
		return (sizeof(struct sockaddr_in6));
	}

	return (sizeof(struct sockaddr_storage));
}

uint16_t
sa_port_get(const struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (0);

	switch (addr->ss_family) {
	case AF_INET:
		return (ntohs(((const struct sockaddr_in*)addr)->sin_port));
	case AF_INET6:
		return (ntohs(((const struct sockaddr_in6*)addr)->sin6_port));
	}

	return (0);
}

int
sa_port_set(struct sockaddr_storage *addr, const uint16_t port) {

	if (NULL == addr)
		return (EINVAL);

	switch (addr->ss_family) {
	case AF_INET:
		((struct sockaddr_in*)addr)->sin_port = htons(port);
		break;
	case AF_INET6:
		((struct sockaddr_in6*)addr)->sin6_port = htons(port);
		break;
	default:
		return (EINVAL);
	}

	return (0);
}

void *
sa_addr_get(struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (NULL);

	switch (addr->ss_family) {
	case AF_INET:
		return (&((struct sockaddr_in*)addr)->sin_addr);
	case AF_INET6:
		return (&((struct sockaddr_in6*)addr)->sin6_addr);
	}

	return (NULL);
}

int
sa_addr_set(struct sockaddr_storage *addr, const void *sin_addr) {

	if (NULL == addr || NULL == sin_addr)
		return (EINVAL);

	switch (addr->ss_family) {
	case AF_INET:
		memcpy(&((struct sockaddr_in*)addr)->sin_addr, sin_addr,
		    sizeof(struct in_addr));
		break;
	case AF_INET6:
		memcpy(&((struct sockaddr_in6*)addr)->sin6_addr, sin_addr,
		    sizeof(struct in6_addr));
		break;
	default:
		return (EINVAL);
	}

	return (0);
}

int
sa_addr_is_specified(const struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (0);

	switch (addr->ss_family) {
	case AF_INET:
		return ((((const struct sockaddr_in*)addr)->sin_addr.s_addr != INADDR_ANY));
	case AF_INET6:
		return (0 == IN6_IS_ADDR_UNSPECIFIED(&((const struct sockaddr_in6*)addr)->sin6_addr));
	}

	return (0);
}

int
sa_addr_is_loopback(const struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (0);

	switch (addr->ss_family) {
	case AF_INET:
		return (IN_LOOPBACK(ntohl(((const struct sockaddr_in*)addr)->sin_addr.s_addr)));
	case AF_INET6:
		return (IN6_IS_ADDR_LOOPBACK(&((const struct sockaddr_in6*)addr)->sin6_addr));
	}

	return (0);
}

int
sa_addr_is_multicast(const struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (0);

	switch (addr->ss_family) {
	case AF_INET:
		return (IN_MULTICAST(ntohl(((const struct sockaddr_in*)addr)->sin_addr.s_addr)));
	case AF_INET6:
		return (IN6_IS_ADDR_MULTICAST(&((const struct sockaddr_in6*)addr)->sin6_addr));
	}

	return (0);
}

int
sa_addr_is_broadcast(const struct sockaddr_storage *addr) {

	if (NULL == addr)
		return (0);

	switch (addr->ss_family) {
	case AF_INET:
		return (IN_BROADCAST(((const struct sockaddr_in*)addr)->sin_addr.s_addr));
	case AF_INET6:
		return (0); /* IPv6 does not have broadcast. */
	}

	return (0);
}


// compares two sockaddr_storage struct, address and port fields
int
sa_addr_port_is_eq(const struct sockaddr_storage *addr1,
    const struct sockaddr_storage *addr2) {

	if (NULL == addr1 || NULL == addr2)
		return (0);
	if (addr1 == addr2)
		return (1);
	if (addr1->ss_family != addr2->ss_family)
		return (0);
	switch (addr1->ss_family) {
	case AF_INET:
		if (((const struct sockaddr_in*)addr1)->sin_port ==
		    ((const struct sockaddr_in*)addr2)->sin_port &&
		    ((const struct sockaddr_in*)addr1)->sin_addr.s_addr ==
		    ((const struct sockaddr_in*)addr2)->sin_addr.s_addr)
			return (1);
		break;
	case AF_INET6:
		if (((const struct sockaddr_in6*)addr1)->sin6_port ==
		    ((const struct sockaddr_in6*)addr2)->sin6_port &&
		    0 == memcmp(
		    &((const struct sockaddr_in6*)addr1)->sin6_addr,
		    &((const struct sockaddr_in6*)addr2)->sin6_addr,
		    sizeof(struct in6_addr)))
			return (1);
		break;
	}
	return (0);
}

// compares two sockaddr_storage struct, ONLY address fields
int
sa_addr_is_eq(const struct sockaddr_storage *addr1,
    const struct sockaddr_storage *addr2) {

	if (NULL == addr1 || NULL == addr2)
		return (0);
	if (addr1 == addr2)
		return (1);
	if (addr1->ss_family != addr2->ss_family)
		return (0);
	switch (addr1->ss_family) {
	case AF_INET:
		if (0 == memcmp(
		    &((const struct sockaddr_in*)addr1)->sin_addr,
		    &((const struct sockaddr_in*)addr2)->sin_addr,
		    sizeof(struct in_addr)))
			return (1);
		break;
	case AF_INET6:
		if (0 == memcmp(
		    &((const struct sockaddr_in6*)addr1)->sin6_addr,
		    &((const struct sockaddr_in6*)addr2)->sin6_addr,
		    sizeof(struct in6_addr)))
			return (1);
		break;
	}
	return (0);
}


/* Ex:
 * 127.0.0.1
 * [2001:4f8:fff6::28]
 * 2001:4f8:fff6::28
 */
int
sa_addr_from_str(struct sockaddr_storage *addr,
    const char *buf, size_t buf_size) {
	size_t addr_size, i;
	char straddr[(STR_ADDR_LEN + 1)];
	const char *ptm, *ptm_end;

	if (NULL == addr || NULL == buf || 0 == buf_size)
		return (EINVAL);

	ptm = buf;
	ptm_end = (buf + buf_size);
	/* Skip spaces, tabs and [ before address. */
	while (ptm < ptm_end && (' ' == (*ptm) || '\t' == (*ptm) || '[' == (*ptm))) {
		ptm ++;
	}
	/* Skip spaces, tabs and ] after address. */
	while (ptm < ptm_end && (' ' == (*(ptm_end - 1)) ||
	    '\t' == (*(ptm_end - 1)) ||
	    ']' == (*(ptm_end - 1)))) {
		ptm_end --;
	}

	addr_size = (size_t)(ptm_end - ptm);
	if (0 == addr_size ||
	    (sizeof(straddr) - 1) < addr_size)
		return (EINVAL);
	memcpy(straddr, ptm, addr_size);
	straddr[addr_size] = 0;

	for (i = 0; i < SIZEOF(family_list); i ++) {
		sa_init(addr, family_list[i], NULL, 0);
		if (inet_pton(family_list[i], straddr, sa_addr_get(addr))) {
			sa_port_set(addr, 0);
			return (0);
		}
	}
	/* Fail: unknown address. */
	return (EINVAL);
}

/* Ex:
 * 127.0.0.1:1234
 * [2001:4f8:fff6::28]:1234
 * 2001:4f8:fff6::28:1234 - wrong, but work.
 */
int
sa_addr_port_from_str(struct sockaddr_storage *addr,
    const char *buf, size_t buf_size) {
	size_t addr_size, i;
	uint16_t port = 0;
	char straddr[(STR_ADDR_LEN + 1)];
	const char *ptm, *ptm_end;

	if (NULL == addr || NULL == buf || 0 == buf_size)
		return (EINVAL);

	ptm = mem_rchr(buf, buf_size, ':'); /* Addr-port delimiter. */
	ptm_end = mem_rchr(buf, buf_size, ']'); /* IPv6 addr end. */
	if (NULL != ptm &&
	    ptm > buf &&
	    ':' != (*(ptm - 1))) { /* IPv6 or port. */
		if (ptm > ptm_end) { /* ptm = port (':' after ']') */
			if (NULL == ptm_end) {
				ptm_end = ptm;
			}
			ptm ++;
			port = str2u16(ptm, (size_t)(buf_size - (size_t)(ptm - buf)));
		}/* else - IPv6 and no port. */
	}
	if (NULL == ptm_end) {
		ptm_end = (buf + buf_size);
	}
	ptm = buf;
	/* Skip spaces, tabs and [ before address. */
	while (ptm < ptm_end && (' ' == (*ptm) || '\t' == (*ptm) || '[' == (*ptm))) {
		ptm ++;
	}
	/* Skip spaces, tabs and ] after address. */
	while (ptm < ptm_end && (' ' == (*(ptm_end - 1)) ||
	    '\t' == (*(ptm_end - 1)) ||
	    ']' == (*(ptm_end - 1)))) {
		ptm_end --;
	}

	addr_size = (size_t)(ptm_end - ptm);
	if (0 == addr_size ||
	    (sizeof(straddr) - 1) < addr_size)
		return (EINVAL);
	memcpy(straddr, ptm, addr_size);
	straddr[addr_size] = 0;

	for (i = 0; i < SIZEOF(family_list); i ++) {
		sa_init(addr, family_list[i], NULL, 0);
		if (inet_pton(family_list[i], straddr, sa_addr_get(addr))) {
			sa_port_set(addr, port);
			return (0);
		}
	}
	/* Fail: unknown address. */
	return (EINVAL);
}

int
sa_addr_to_str(const struct sockaddr_storage *addr, char *buf,
    size_t buf_size, size_t *buf_size_ret) {
	int error = 0;

	if (NULL == addr || NULL == buf || 0 == buf_size)
		return (EINVAL);

	buf_size --; /* Allways keep space. */
	switch (addr->ss_family) {
	case AF_INET:
		if (NULL == inet_ntop(AF_INET,
		    &((const struct sockaddr_in*)addr)->sin_addr,
		    buf, buf_size)) {
			buf[0] = 0;
			error = errno;
		}
		break;
	case AF_INET6:
		if (NULL == inet_ntop(AF_INET6,
		    &((const struct sockaddr_in6*)addr)->sin6_addr,
		    buf, buf_size)) {
			buf[0] = 0;
			error = errno;
		}
		break;
	default:
		buf[0] = 0;
		error = EINVAL;
		break;
	}
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = strnlen(buf, buf_size);
	}

	return (error);
}

int
sa_addr_port_to_str(const struct sockaddr_storage *addr, char *buf,
    size_t buf_size, size_t *buf_size_ret) {
	int error = 0;
	size_t size_ret = 0;

	if (NULL == addr || NULL == buf || 0 == buf_size)
		return (EINVAL);

	buf_size --; /* Allways keep space. */
	switch (addr->ss_family) {
	case AF_INET:
		if (NULL == inet_ntop(AF_INET,
		    &((const struct sockaddr_in*)addr)->sin_addr,
		    buf, buf_size)) {
			buf[0] = 0;
			error = errno;
			break;
		}
		size_ret = strnlen(buf, buf_size);
		if (0 != ((const struct sockaddr_in*)addr)->sin_port) {
			size_ret += (size_t)snprintf((buf + size_ret),
			    (size_t)(buf_size - size_ret), ":%hu",
			    ntohs(((const struct sockaddr_in*)addr)->sin_port));
		}
		break;
	case AF_INET6:
		if (NULL == inet_ntop(AF_INET6,
		    &((const struct sockaddr_in6*)addr)->sin6_addr,
		    (buf + 1), (buf_size - 1))) {
			buf[0] = 0;
			error = errno;
			break;
		}
		buf[0] = '[';
		size_ret = strnlen(buf, buf_size);
		if (0 != ((const struct sockaddr_in6*)addr)->sin6_port) {
			size_ret += (size_t)snprintf((buf + size_ret),
			    (size_t)(buf_size - size_ret), "]:%hu",
			    ntohs(((const struct sockaddr_in6*)addr)->sin6_port));
		} else {
			buf[size_ret] = ']';
			buf[(size_ret + 1)] = 0;
			size_ret += 2;
		}
		break;
	default:
		buf[0] = 0;
		error = EINVAL;
		break;
	}
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = size_ret;
	}

	return (error);
}
