/*-
 * Copyright (c) 2013 - 2020 Rozhuk Ivan <rozhuk.im@gmail.com>
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


#ifndef __UPNP_SSDP_H__
#define __UPNP_SSDP_H__

#include <sys/types.h>
#include <inttypes.h>
#include "threadpool/threadpool.h"
#include "net/socket_options.h"


typedef struct upnp_ssdp_dev_s *upnp_ssdp_dev_p;
typedef struct upnp_ssdp_s *upnp_ssdp_p;


typedef struct upnp_ssdp_settings_s { /* Settings. */
	skt_opts_t	skt_opts;
	uint16_t	search_port;	/* UPnP 1.1: SEARCHPORT.UPNP.ORG: (49152-65535) identifies the port at which the device listens to unicast M-SEARCH messages */
	uint32_t	flags;		/* Flags. */
	size_t		http_server_size; /* 'OS/version UPnP/1.1 product/version' len. */
	char		http_server[256]; /* 'OS/version UPnP/1.1 product/version' */
} upnp_ssdp_settings_t, *upnp_ssdp_settings_p;
#define UPNP_SSDP_S_F_IPV4		(((uint32_t)1) << 0) /* Enable IPv4 support. */
#define UPNP_SSDP_S_F_IPV6		(((uint32_t)1) << 1) /* Enable IPv6 support. */
#define UPNP_SSDP_S_F_BYEBYE		(((uint32_t)1) << 2) /* Send byebye on shutdown. */

#define UPNP_SSDP_S_SKT_OPTS_LOAD_MASK	(SO_F_RCVBUF |			\
					SO_F_SNDBUF |			\
					SO_F_IP_HOPLIM_U |		\
					SO_F_IP_HOPLIM_M)

/* Default values. */
#define UPNP_SSDP_S_DEF_SKT_OPTS_MASK	(SO_F_IP_HOPLIM_U | SO_F_IP_HOPLIM_M) /* Opts that have def values. */
#define UPNP_SSDP_S_DEF_SKT_OPTS_VALS	(0)
#define UPNP_SSDP_DEF_HOP_LIMIT		(1)	/* Count, same default for unicast and multicast. */
#define UPNP_SSDP_DEF_SEARCH_PORT	(1900)	/* Number. */
#define UPNP_SSDP_DEF_MAX_AGE		(1800)	/* sec. */
#define UPNP_SSDP_DEF_ANNOUNCE_INTERVAL	(60)	/* sec. */
#define UPNP_SSDP_DEF_FLAGS		(UPNP_SSDP_S_F_BYEBYE) /* IPv4 and IPv6. */


void	upnp_ssdp_def_settings(upnp_ssdp_settings_p s_ret);
int	upnp_ssdp_create(tp_p tp, upnp_ssdp_settings_p s, upnp_ssdp_p *ssdp_ret);
void	upnp_ssdp_destroy(upnp_ssdp_p ssdp);


int	upnp_ssdp_dev_add(upnp_ssdp_p ssdp, const char *uuid, 
	    const char *domain_name, size_t domain_name_size,
	    const char *type, size_t type_size, const uint32_t ver,
	    uint32_t boot_id, uint32_t config_id,
	    uint32_t max_age, uint32_t ann_interval, upnp_ssdp_dev_p *dev_ret);
void	upnp_ssdp_dev_del(upnp_ssdp_p ssdp, upnp_ssdp_dev_p dev);
size_t	upnp_ssdp_root_dev_count(upnp_ssdp_p ssdp);

int	upnp_ssdp_svc_add(upnp_ssdp_dev_p dev,
	    const char *domain_name, size_t domain_name_size,
	    const char *type, size_t type_size, const uint32_t ver);

int	upnp_ssdp_dev_if_add(upnp_ssdp_p ssdp, upnp_ssdp_dev_p dev,
	    const char *if_name, size_t if_name_size,
	    const char *url4, size_t url4_size,
	    const char *url6, size_t url6_size);
size_t	upnp_ssdp_if_count(upnp_ssdp_p ssdp);

void	upnp_ssdp_send_notify(upnp_ssdp_p ssdp);


#endif /* __UPNP_SSDP_H__ */
