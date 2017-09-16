/*-
 * Copyright (c) 2011 - 2016 Rozhuk Ivan <rozhuk.im@gmail.com>
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



#ifndef __CORE_NET_HELPERS_H__
#define __CORE_NET_HELPERS_H__

#include <sys/param.h>

#ifdef __linux__ /* Linux specific code. */
#	define _GNU_SOURCE /* See feature_test_macros(7) */
#	define __USE_GNU 1
#endif /* Linux specific code. */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define STR_ADDR_LEN		(56) /* 46(INET6_ADDRSTRLEN) + 2('[]') + 1(':') + 5(port num) + zero */


#ifndef s6_addr32
#define s6_addr32		__u6_addr.__u6_addr32
#endif

#ifndef ifr_ifindex
#define ifr_ifindex		ifr_ifru.ifru_index
#endif


#ifndef IN_LOOPBACK
#define IN_LOOPBACK(__x)	(((u_int32_t)(__x) & 0xff000000) == 0x7f000000)
#endif

#ifndef IN_BROADCAST
#define IN_BROADCAST(__x)	((u_int32_t)(__x) == INADDR_BROADCAST)
#endif

#ifndef IN_MULTICAST
#define IN_MULTICAST(__x)	(((u_int32_t)(__x) & 0xf0000000) == 0xe0000000)
#endif

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(__x) ((__x)->s6_addr[0] == 0xff)
#endif



void	sa_copy(const void *src, void *dst);
void	sa_init(struct sockaddr_storage *addr, int family,
	    void *sin_addr, uint16_t port);
socklen_t sa_type2size(const struct sockaddr_storage *addr);
uint16_t sa_port_get(const struct sockaddr_storage *addr);
void	sa_port_set(struct sockaddr_storage *addr, uint16_t port);
int	sa_addr_is_specified(const struct sockaddr_storage *addr);
int	sa_addr_is_loopback(const struct sockaddr_storage *addr);
int	sa_addr_is_multicast(const struct sockaddr_storage *addr);
int	sa_addr_is_broadcast(const struct sockaddr_storage *addr);
int	sa_addr_port_is_eq(const struct sockaddr_storage *addr1,
	    const struct sockaddr_storage *addr2);
int	sa_addr_is_eq(const struct sockaddr_storage *addr1,
	    const struct sockaddr_storage *addr2);

int	sa_addr_from_str(struct sockaddr_storage *addr,
	    const char *buf, size_t buf_size);
int	sa_addr_port_from_str(struct sockaddr_storage *addr,
	    const char *buf, size_t buf_size);
int	sa_addr_to_str(const struct sockaddr_storage *addr,
	    char *buf, size_t buf_size, size_t *buf_size_ret);
int	sa_addr_port_to_str(const struct sockaddr_storage *addr,
	     char *buf, size_t buf_size, size_t *buf_size_ret);

int	str_net_to_ss(const char *buf, size_t buf_size, struct sockaddr_storage *addr,
	    uint16_t *preflen_ret);
void	net_addr_truncate_preflen(struct sockaddr_storage *net_addr, uint16_t preflen);
void	net_addr_truncate_mask(int family, uint32_t *net, uint32_t *mask);
int	is_addr_in_net(int family, const uint32_t *net, const uint32_t *mask,
	    const uint32_t *addr);

int	inet_len2mask(size_t len, struct in_addr *mask);
int	inet_mask2len(const struct in_addr *mask);

int	inet6_len2mask(size_t len, struct in6_addr *mask);
int	inet6_mask2len(const struct in6_addr *mask);

int	get_if_addr_by_name(const char *if_name, size_t if_name_size, sa_family_t family,
	    struct sockaddr_storage *addr);
int	get_if_addr_by_idx(uint32_t if_index, sa_family_t family,
	    struct sockaddr_storage *addr);
int	is_host_addr(const struct sockaddr_storage *addr);
int	is_host_addr_ex(const struct sockaddr_storage *addr, void **data);
void	is_host_addr_ex_free(void *data);

size_t	iovec_calc_size(struct iovec *iov, size_t iov_cnt);
void	iovec_set_offset(struct iovec *iov, size_t iov_cnt, size_t iov_off);


static inline void
sain4_init(struct sockaddr_storage *addr) {

	memset(addr, 0, sizeof(struct sockaddr_in));
#ifdef BSD /* BSD specific code. */
	((struct sockaddr_in*)addr)->sin_len = sizeof(struct sockaddr_in);
#endif /* BSD specific code. */
	((struct sockaddr_in*)addr)->sin_family = AF_INET;
	//addr->sin_port = 0;
	//addr->sin_addr.s_addr = 0;
}

static inline void
sain4_p_set(struct sockaddr_storage *addr, uint16_t port) {
	((struct sockaddr_in*)addr)->sin_port = htons(port);
}

static inline uint16_t
sain4_p_get(const struct sockaddr_storage *addr) {
	return (ntohs(((const struct sockaddr_in*)addr)->sin_port));
}

static inline void
sain4_a_set(struct sockaddr_storage *addr, void *sin_addr) {
	memcpy(&((struct sockaddr_in*)addr)->sin_addr, sin_addr,
	    sizeof(struct in_addr));
}

static inline void
sain4_a_set_val(struct sockaddr_storage *addr, uint32_t sin_addr) {
	((struct sockaddr_in*)addr)->sin_addr.s_addr = sin_addr;
}


static inline void
sain4_astr_set(struct sockaddr_storage *addr, const char *straddr) {
	((struct sockaddr_in*)addr)->sin_addr.s_addr = inet_addr(straddr);
}


static inline void
sain6_init(struct sockaddr_storage *addr) {

	memset(addr, 0, sizeof(struct sockaddr_in6));
#ifdef BSD /* BSD specific code. */
	((struct sockaddr_in6*)addr)->sin6_len = sizeof(struct sockaddr_in6);
#endif /* BSD specific code. */
	((struct sockaddr_in6*)addr)->sin6_family = AF_INET6;
	//((struct sockaddr_in6*)addr)->sin6_port = 0;
	//((struct sockaddr_in6*)addr)->sin6_flowinfo = 0;
	//((struct sockaddr_in6*)addr)->sin6_addr
	//((struct sockaddr_in6*)addr)->sin6_scope_id = 0;
}

static inline void
sain6_p_set(struct sockaddr_storage *addr, uint16_t port) {
	((struct sockaddr_in6*)addr)->sin6_port = htons(port);
}

static inline uint16_t
sain6_p_get(const struct sockaddr_storage *addr) {
	return (ntohs(((const struct sockaddr_in6*)addr)->sin6_port));
}

static inline void
sain6_a_set(struct sockaddr_storage *addr, void *sin6_addr) {
	memcpy(&((struct sockaddr_in6*)addr)->sin6_addr, sin6_addr,
	    sizeof(struct in6_addr));
}



#endif /* __CORE_NET_HELPERS_H__ */
