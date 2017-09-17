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


#ifndef __NET_SOCKET_ADDRESS_H__
#define __NET_SOCKET_ADDRESS_H__

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


void	sa_copy(const void *src, void *dst);
int	sa_init(struct sockaddr_storage *addr, const sa_family_t family,
	    const void *sin_addr, const uint16_t port);
sa_family_t sa_family(const struct sockaddr_storage *addr);
socklen_t sa_size(const struct sockaddr_storage *addr);
uint16_t sa_port_get(const struct sockaddr_storage *addr);
int	sa_port_set(struct sockaddr_storage *addr, const uint16_t port);
void 	*sa_addr_get(struct sockaddr_storage *addr);
int	sa_addr_set(struct sockaddr_storage *addr, const void *sin_addr);
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


#endif /* __NET_SOCKET_ADDRESS_H__ */
