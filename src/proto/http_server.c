/*-
 * Copyright (c) 2011-2025 Rozhuk Ivan <rozhuk.im@gmail.com>
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
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include <inttypes.h>
#include <stdlib.h> /* malloc, exit */
#include <unistd.h> /* close, write, sysconf */
#include <string.h> /* memcpy, memmove, memset, strerror... */
#include <strings.h> /* explicit_bzero */
#include <stdio.h> /* snprintf, fprintf */
#include <time.h>
#include <errno.h>

#include "utils/macro.h"
#include "utils/mem_utils.h"
#include "utils/num2str.h"
#include "utils/str2num.h"
#include "proto/http.h"

#include "threadpool/threadpool_task.h"
#include "net/socket.h"
#include "net/socket_address.h"
#include "net/utils.h"
#include "utils/info.h"
#include "net/hostname_list.h"
#include "proto/http_server.h"
#ifdef HTTP_SRV_XML_CONFIG
#	include "utils/buf_str.h"
#	include "utils/xml.h"
#endif


#define HTTP_LIB_NAME			"HTTP core server by Rozhuk Ivan"
#define HTTP_LIB_VER			"1.7"
#define HTTP_LIB_NAME_VER		HTTP_LIB_NAME"/"HTTP_LIB_VER

#define HTTP_SRV_ALLOC_CNT		8




typedef struct http_srv_bind_s {
	tp_task_p	*tptasks;	/* Accept incomming task. */
	size_t		tptasks_cnt;
	http_srv_p	srv;		/* HTTP server */
	void		*udata;		/* Acceptor associated data. */
	hostname_list_t	hst_name_lst;	/* List of host names on this bind. */
	http_srv_bind_settings_t s;	/* settings */
} http_srv_bind_t;



typedef struct http_srv_s {
	tp_p			tp;
	http_srv_on_conn_cb	on_conn; /* New client connected callback */
	http_srv_cli_ccb_t	ccb;	/* Default client callbacks. */
	void			*udata;	/* Server associated data. */
	http_srv_stat_t		stat;
	size_t			bind_count;
	size_t			bind_allocated;
	http_srv_bind_p		*bnd;	/* Acceptors pointers array. */
	hostname_list_t		hst_name_lst;	/* List of host names on this server. */
	http_srv_settings_t	s;	/* settings */
} http_srv_t;



typedef struct http_srv_cli_s {
	tp_task_p		tptask;	/* recv/send from/to client, and socket container. */
	io_buf_p		rcv_buf;/* Used for receive http request only. */
	io_buf_p		buf;	/* Used for send http responce only. */
	http_srv_bind_p		bnd;	/*  */
	http_srv_req_t		req;	/* Parsed request data. */
	http_srv_resp_t		resp;	/* Responce data. */
	http_srv_cli_ccb_t	ccb;	/* Custom client callbacks. */
	void			*udata;	/* Client associated data. */
	uint32_t		flags;	/* Flags: HTTP_SRV_CLI_F_*. */
	uint32_t		flags_int; /* Flags: HTTP_SRV_CLI_FI_*. */
	sockaddr_storage_t addr;	/* Client address. */
} http_srv_cli_t;

#define HTTP_SRV_CLI_FI_NEXT_BYTE_MASK	((uint32_t)0x000000ff)
#define HTTP_SRV_CLI_FI_NEXT_BYTE_SET	(((uint32_t)1) << 9) /* Keep here byte value from next request. */


http_srv_cli_p	http_srv_cli_alloc(http_srv_bind_p bnd, tpt_p tpt,
		    uintptr_t skt, http_srv_cli_ccb_p ccb, void *udata);
void		http_srv_cli_free(http_srv_cli_p cli);

static int	http_srv_new_conn_cb(tp_task_p tptask, int error, uintptr_t skt,
		    sockaddr_storage_p addr, void *arg);
static int	http_srv_recv_done_cb(tp_task_p tptask, int error, io_buf_p buf,
		    uint32_t eof, size_t transfered_size, void *arg);
static int	http_srv_send_responce(http_srv_cli_p cli, const uint8_t **delimiter);
static int	http_srv_snd_done_cb(tp_task_p tptask, int error, io_buf_p buf,
		    uint32_t eof, size_t transfered_size, void *arg);

/*
 * resp_p_flags - HTTP_SRV_RESP_P_F_*
 */
static int	http_srv_snd(http_srv_cli_p cli);



void
http_srv_def_settings(int add_os_ver, const char *app_ver, int add_lib_ver,
    http_srv_settings_p s_ret) {
	size_t app_ver_size;

	if (NULL == s_ret)
		return;
	/* Init. */
	memset(s_ret, 0x00, sizeof(http_srv_settings_t));
	skt_opts_init(HTTP_SRV_S_SKT_OPTS_INT_MASK,
	    HTTP_SRV_S_SKT_OPTS_INT_VALS, &s_ret->skt_opts);
	s_ret->skt_opts.mask |= SO_F_NONBLOCK;
	s_ret->skt_opts.bit_vals |= SO_F_NONBLOCK;
#ifdef SO_ACCEPTFILTER
	memcpy(s_ret->skt_opts.tcp_acc_filter.af_name, 
	    HTTP_SRV_S_SKT_OPTS_ACC_FILTER_NAME,
	    sizeof(HTTP_SRV_S_SKT_OPTS_ACC_FILTER_NAME));
#elif defined(TCP_DEFER_ACCEPT)
	s_ret->skt_opts.tcp_acc_defer = HTTP_SRV_S_SKT_OPTS_ACC_FILTER_DEFER;
#endif

	/* Default settings. */
	s_ret->skt_opts.mask |= HTTP_SRV_S_DEF_SKT_OPTS_MASK;
	s_ret->skt_opts.bit_vals |= HTTP_SRV_S_DEF_SKT_OPTS_VALS;
	s_ret->skt_opts.rcv_timeout = HTTP_SRV_S_DEF_SKT_OPTS_RCVTIMEO;
	s_ret->skt_opts.snd_timeout = HTTP_SRV_S_DEF_SKT_OPTS_SNDTIMEO;
	s_ret->rcv_io_buf_init_size = HTTP_SRV_S_DEF_RCV_IO_BUF_INIT;
	s_ret->rcv_io_buf_max_size = HTTP_SRV_S_DEF_RCV_IO_BUF_MAX;
	s_ret->snd_io_buf_init_size = HTTP_SRV_S_DEF_SND_IO_BUF_INIT;
	s_ret->hdrs_reserve_size = HTTP_SRV_S_DEF_HDRS_SIZE;
	s_ret->req_p_flags = HTTP_SRV_S_DEF_RQ_P_FLAGS;
	s_ret->resp_p_flags = HTTP_SRV_S_DEF_RESP_P_FLAGS;

	/* 'OS/version UPnP/1.1 product/version' */
	s_ret->http_server_size = 0;
	if (0 != add_os_ver) {
		if (0 != info_get_os_ver("/", 1, s_ret->http_server,
		    (sizeof(s_ret->http_server) - 1),
		    &s_ret->http_server_size)) {
			memcpy(s_ret->http_server, "Generic OS/1.0", 15);
			s_ret->http_server_size = 14;
		}
	}
	if (NULL != app_ver &&
	    (sizeof(s_ret->http_server) - 2) > (app_ver_size = strlen(app_ver))) {
		if (0 != s_ret->http_server_size) {
			s_ret->http_server[s_ret->http_server_size ++] = ' ';
		}
		memcpy(s_ret->http_server, app_ver, app_ver_size);
		s_ret->http_server_size = app_ver_size;
	}
	if (0 != add_lib_ver &&
	    (sizeof(s_ret->http_server) - 2) > sizeof(HTTP_LIB_NAME_VER)) {
		if (0 != s_ret->http_server_size) {
			s_ret->http_server[s_ret->http_server_size ++] = ' ';
		}
		memcpy(s_ret->http_server, HTTP_LIB_NAME_VER,
		    sizeof(HTTP_LIB_NAME_VER));
		s_ret->http_server_size = (sizeof(HTTP_LIB_NAME_VER) - 1);
	}
	s_ret->http_server[s_ret->http_server_size] = 0;
}

void
http_srv_bind_def_settings(skt_opts_p skt_opts, http_srv_bind_settings_p s_ret) {

	if (NULL == s_ret)
		return;
	memset(s_ret, 0x00, sizeof(http_srv_bind_settings_t));
	/* default settings */
	memcpy(&s_ret->skt_opts, skt_opts, sizeof(skt_opts_t));
}

#ifdef HTTP_SRV_XML_CONFIG
int
http_srv_xml_load_hostnames(const uint8_t *buf, size_t buf_size,
    hostname_list_p hst_name_lst) {
	int error;
	const uint8_t *data, *cur_pos;
	size_t data_size;
	char strbuf[256];

	if (NULL == buf || 0 == buf_size || NULL == hst_name_lst)
		return (EINVAL);
	/* Read hostnames. */
	hostname_list_init(hst_name_lst);
	cur_pos = NULL;
	while (0 == xml_get_val_args(buf, buf_size, &cur_pos, NULL, NULL,
	    &data, &data_size, (const uint8_t*)"hostnameList", "hostname", NULL)) {
		error = hostname_list_add(hst_name_lst, data, data_size);
		data_size = MIN(data_size, (sizeof(strbuf) - 1));
		memcpy(strbuf, data, data_size);
		strbuf[data_size] = 0;
		if (0 != error) {
			SYSLOG_ERR(LOG_WARNING, error,
			    "hostname_list_add(%s).", strbuf);
			continue;
		}
		syslog(LOG_INFO, "hostname: %s", strbuf);
	}
	return (0);
}

int
http_srv_xml_load_settings(const uint8_t *buf, size_t buf_size,
    http_srv_settings_p s) {
	const uint8_t *data;
	size_t data_size;

	if (NULL == buf || 0 == buf_size || NULL == s)
		return (EINVAL);
	/* Read from config. */
	/* Socket options. */
	if (0 == xml_get_val_args(buf, buf_size, NULL, NULL, NULL,
	    &data, &data_size, (const uint8_t*)"skt", NULL)) {
		skt_opts_xml_load(data, data_size,
		    HTTP_SRV_S_SKT_OPTS_LOAD_MASK, &s->skt_opts);
	}
	xml_get_val_size_t_args(buf, buf_size, NULL, &s->rcv_io_buf_init_size,
	    (const uint8_t*)"ioBufInitSize", NULL);
	xml_get_val_size_t_args(buf, buf_size, NULL, &s->rcv_io_buf_max_size,
	    (const uint8_t*)"ioBufMaxSize", NULL);
	return (0);
}
	
int
http_srv_xml_load_bind(const uint8_t *buf, size_t buf_size,
    http_srv_bind_settings_p s) {
	int error;
	const uint8_t *data;
	size_t data_size;
	uint16_t tm16;
	char straddr[STR_ADDR_LEN];

	if (NULL == buf || 0 == buf_size || NULL == s)
		return (EINVAL);
	/* address */
	if (0 != xml_get_val_args(buf, buf_size, NULL, NULL, NULL,
	    &data, &data_size, (const uint8_t*)"address", NULL)) {
		syslog(LOG_CRIT, "HTTP server: server addr not set.");
		return (EINVAL);
	}
	if (0 != sa_addr_port_from_str(&s->addr, (const char*)data, data_size)) {
		memcpy(straddr, data, MIN((sizeof(straddr) - 1), data_size));
		straddr[MIN((sizeof(straddr) - 1), data_size)] = 0;
		syslog(LOG_CRIT, "HTTP server: invalid addr: %s (len=%zu).",
		    straddr, data_size);
		return (EINVAL);
	}
	if (0 == xml_get_val_args(buf, buf_size, NULL, NULL, NULL,
	    &data, &data_size, (const uint8_t*)"ifName", NULL)) {
		tm16 = sa_port_get(&s->addr);
		error = get_if_addr_by_name((const char*)data, data_size,
		    s->addr.ss_family, &s->addr);
		if (0 != error) {
			memcpy(straddr, data, MIN((sizeof(straddr) - 1), data_size));
			straddr[MIN((sizeof(straddr) - 1), data_size)] = 0;
			SYSLOG_ERR(LOG_CRIT, error,
			    "HTTP server: cant get addr for: %s.", straddr);
			return (error);
		}
		sa_port_set(&s->addr, tm16);
	}
	/* Socket options. */
	skt_opts_xml_load(buf, buf_size, HTTP_SRV_S_SKT_OPTS_LOAD_MASK, &s->skt_opts);

	return (0);
}


int
http_srv_xml_load_start(const uint8_t *buf, size_t buf_size, tp_p tp,
    http_srv_on_conn_cb on_conn, http_srv_cli_ccb_p ccb,
    http_srv_settings_p srv_settings, void *udata,
    http_srv_p *http_srv) {
	int error;
	const uint8_t *data, *cur_pos;
	size_t data_size;
	char straddr[STR_ADDR_LEN];
	http_srv_settings_t srv_s;
	http_srv_bind_settings_t bind_s;
	hostname_list_t hst_name_lst;

	if (NULL == buf || 0 == buf_size || NULL == tp ||
	    NULL == srv_settings || NULL == http_srv)
		return (EINVAL);

	/* HTTP server settings. */
	if (0 == xml_calc_tag_count_args(buf, buf_size,
	    (const uint8_t*)"bindList", "bind", NULL))
		goto no_http_svr;
	/* Default settings. */
	memcpy(&srv_s, srv_settings, sizeof(srv_s));
	/* Read from config. */
	http_srv_xml_load_settings(buf, buf_size, &srv_s);
	/* Read hostnames. */
	hostname_list_init(&hst_name_lst);
	http_srv_xml_load_hostnames(buf, buf_size, &hst_name_lst);

	error = http_srv_create(tp, on_conn, ccb, &hst_name_lst,
	    &srv_s, udata, http_srv);
	if (0 != error) {
		SYSLOG_ERR(LOG_CRIT, error, "http_srv_create().");
		hostname_list_deinit(&hst_name_lst);
		return (error);
	}

	/* Load and add servers. */
	cur_pos = NULL;
	while (0 == xml_get_val_args(buf, buf_size, &cur_pos, NULL, NULL,
	    &data, &data_size, (const uint8_t*)"bindList", "bind", NULL)) {
		http_srv_bind_def_settings(&srv_s.skt_opts, &bind_s);
		error = http_srv_xml_load_bind(data, data_size, &bind_s);
		if (0 != error) {
			SYSLOG_ERR(LOG_ERR, error, "http_srv_xml_load_bind().");
			continue;
		}
		/* Read hostnames. */
		hostname_list_init(&hst_name_lst);
		http_srv_xml_load_hostnames(data, data_size, &hst_name_lst);

		sa_addr_port_to_str(&bind_s.addr, straddr, sizeof(straddr), NULL);
		/* Try bind... */
		error = http_srv_bind_add((*http_srv), &bind_s, &hst_name_lst, NULL, NULL);
		if (0 != error) {
			SYSLOG_ERR(LOG_ERR, error, "http_srv_bind_add(): %s,"
			    " backlog = %i, tcp_cc = %s.",
			    straddr, bind_s.skt_opts.backlog, bind_s.skt_opts.tcp_cc);
			continue;
		}
		syslog(LOG_INFO, "bind %s, backlog = %i, tcp_cc = %s.",
		    straddr, bind_s.skt_opts.backlog, bind_s.skt_opts.tcp_cc);
	}
	if (0 == http_srv_get_bind_count((*http_srv))) {
no_http_svr:
		syslog(LOG_NOTICE, "no bind address specified, nothink to do...");
		return (EINVAL);
	}

	return (0);
}
#endif /* HTTP_SRV_XML_CONFIG */



int
http_srv_create(tp_p tp, http_srv_on_conn_cb on_conn,
    http_srv_cli_ccb_p ccb, hostname_list_p hst_name_lst,
    http_srv_settings_p s, void *udata, http_srv_p *srv_ret) {
	int error;
	http_srv_p srv = NULL;
	struct timespec ts;

	SYSLOGD_EX(LOG_DEBUG, "...");
	
	if (NULL == srv_ret) {
		error = EINVAL;
		goto err_out;
	}
	if (NULL != s) { /* Validate settings. */
		if ((0 != s->snd_io_buf_init_size &&
		     (s->hdrs_reserve_size + HTTP_SRV_S_DEF_HDRS_SIZE) > s->snd_io_buf_init_size) ||
		    sizeof(s->http_server) <= s->http_server_size) {
			error = EINVAL;
			goto err_out;
		}
	}
	/* Create. */
	srv = calloc(1, sizeof(http_srv_t));
	if (NULL == srv) {
		error = ENOMEM;
		goto err_out;
	}
	srv->tp = tp;
	srv->on_conn = on_conn;
	if (NULL != ccb) {
		srv->ccb = (*ccb); /* memcpy */
	}
	srv->udata = udata;
	if (NULL != hst_name_lst) {
		memcpy(&srv->hst_name_lst, hst_name_lst, sizeof(hostname_list_t));
	}
	if (NULL == s) { /* Apply default settings */
		http_srv_def_settings(1, NULL, 1, &srv->s);
	} else {
		memcpy(&srv->s, s, sizeof(http_srv_settings_t));
	}
	/* kb -> bytes, sec -> msec */
	skt_opts_cvt(SKT_OPTS_MULT_K, &srv->s.skt_opts);
	srv->s.rcv_io_buf_init_size *= 1024;
	srv->s.rcv_io_buf_max_size *= 1024;
	srv->s.snd_io_buf_init_size *= 1024;
	srv->s.hdrs_reserve_size *= 1024;
	srv->s.http_server[srv->s.http_server_size] = 0;

	clock_gettime(CLOCK_REALTIME_FAST, &ts);
	srv->stat.start_time = ts.tv_sec;
	clock_gettime(CLOCK_MONOTONIC_FAST, &ts);
	srv->stat.start_time_abs = ts.tv_sec;

	(*srv_ret) = srv;
	return (0);

err_out:
	hostname_list_deinit(hst_name_lst);
	if (NULL != srv) {
		free(srv);
	}
	/* Error. */
	SYSLOG_ERR_EX(LOG_ERR, error, "err_out.");

	return (error);
}

void
http_srv_shutdown(http_srv_p srv) {
	size_t i;

	SYSLOGD_EX(LOG_DEBUG, "...");
	if (NULL == srv || NULL != srv->bnd)
		return;
	for (i = 0; i < srv->bind_count; i ++) {
		http_srv_bind_shutdown(srv->bnd[i]);
	}
}

void
http_srv_destroy(http_srv_p srv) {
	size_t i;

	SYSLOGD_EX(LOG_DEBUG, "...");
	if (NULL == srv)
		return;
	if (NULL != srv->bnd) {
		for (i = 0; i < srv->bind_count; i ++) {
			http_srv_bind_remove(srv->bnd[i]);
		}
		free(srv->bnd);
	}
	hostname_list_deinit(&srv->hst_name_lst);
	free(srv);
}

size_t
http_srv_get_bind_count(http_srv_p srv) {

	if (NULL == srv)
		return (0);
	return (srv->bind_count);
}

int
http_srv_stat_get(http_srv_p srv, http_srv_stat_p stat) {

	if (NULL == srv || NULL == stat)
		return (EINVAL);
	memcpy(stat, &srv->stat, sizeof(http_srv_stat_t));
	return (0);
}

tp_p
http_srv_tp_get(http_srv_p srv) {

	if (NULL == srv)
		return (NULL);
	return (srv->tp);
}

int
http_srv_tp_set(http_srv_p srv, tp_p tp) {

	if (NULL == srv)
		return (EINVAL);
	srv->tp = tp;
	return (0);
}

int
http_srv_on_conn_cb_set(http_srv_p srv, http_srv_on_conn_cb on_conn) {

	if (NULL == srv)
		return (EINVAL);
	srv->on_conn = on_conn;
	return (0);
}

int
http_srv_ccb_get(http_srv_p srv, http_srv_cli_ccb_p ccb) {

	if (NULL == srv || NULL == ccb)
		return (EINVAL);
	(*ccb) = srv->ccb;
	return (0);
}

int
http_srv_ccb_set(http_srv_p srv, http_srv_cli_ccb_p ccb) {

	if (NULL == srv || NULL == ccb)
		return (EINVAL);
	srv->ccb = (*ccb);
	return (0);
}

int
http_srv_on_destroy_cb_set(http_srv_p srv, http_srv_on_destroy_cb on_destroy) {

	if (NULL == srv)
		return (EINVAL);
	srv->ccb.on_destroy = on_destroy;
	return (0);
}

int
http_srv_on_req_rcv_cb_set(http_srv_p srv, http_srv_on_req_rcv_cb on_req_rcv) {

	if (NULL == srv)
		return (EINVAL);
	srv->ccb.on_req_rcv = on_req_rcv;
	return (0);
}

int
http_srv_on_rep_snd_cb_set(http_srv_p srv, http_srv_on_resp_snd_cb on_rep_snd) {

	if (NULL == srv)
		return (EINVAL);
	srv->ccb.on_rep_snd = on_rep_snd;
	return (0);
}

void *
http_srv_get_udata(http_srv_p srv) {

	if (NULL == srv)
		return (NULL);
	return (srv->udata);
}

int
http_srv_set_udata(http_srv_p srv, void *udata) {

	if (NULL == srv)
		return (EINVAL);
	srv->udata = udata;
	return (0);
}


/* HTTP Acceptor */
int
http_srv_bind_add(http_srv_p srv, http_srv_bind_settings_p s,
    hostname_list_p hst_name_lst, void *udata, http_srv_bind_p *bind_ret) {
	int error;
	http_srv_bind_p bnd = NULL;

	SYSLOGD_EX(LOG_DEBUG, "...");
	if (NULL == srv || NULL == s)
		return (EINVAL);

	bnd = calloc(1, sizeof(http_srv_bind_t));
	if (NULL == bnd)
		return (ENOMEM);
	bnd->srv = srv;
	memcpy(&bnd->s, s, sizeof(http_srv_bind_settings_t));
	bnd->udata = udata;
	if (NULL != hst_name_lst) {
		memcpy(&bnd->hst_name_lst, hst_name_lst, sizeof(hostname_list_t));
	}
	/* kb -> bytes, sec -> msec */
	skt_opts_cvt(SKT_OPTS_MULT_K, &bnd->s.skt_opts);

	/* Create listen sockets per thread or on one on rand thread. */
	error = tp_task_bind_accept_multi_create(srv->tp,
	    &bnd->s.addr, SOCK_STREAM, IPPROTO_TCP, &bnd->s.skt_opts,
	    TP_TASK_F_CLOSE_ON_DESTROY, 0, http_srv_new_conn_cb, bnd,
	    &bnd->tptasks_cnt, &bnd->tptasks);
	if (0 != error)
		goto err_out;

	/* Link server and acceptor. */
	error = realloc_items((void**)&srv->bnd, sizeof(http_srv_bind_p),
	    &srv->bind_allocated, HTTP_SRV_ALLOC_CNT, srv->bind_count);
	if (0 != error) /* Realloc fail! */
		goto err_out;
	srv->bnd[srv->bind_count] = bnd;
	srv->bind_count ++;
	if (NULL != bind_ret) {
		(*bind_ret) = bnd;
	}
	return (0);

err_out:
	/* Error. */
	bnd->srv = NULL;
	http_srv_bind_remove(bnd);
	SYSLOG_ERR_EX(LOG_ERR, error, "err_out.");

	return (error);
}

void
http_srv_bind_shutdown(http_srv_bind_p bnd) {
	size_t i;

	SYSLOGD_EX(LOG_DEBUG, "...");
	if (NULL == bnd)
		return;
	for (i = 0; i < bnd->tptasks_cnt; i ++) {
		tp_task_ident_close(bnd->tptasks[i]);
	}
}

void
http_srv_bind_remove(http_srv_bind_p bnd) {
	size_t i;
	http_srv_p srv;

	SYSLOGD_EX(LOG_DEBUG, "...");
	if (NULL == bnd)
		return;
	if (NULL != bnd->srv) {
		srv = bnd->srv;
		for (i = 0; i < srv->bind_count; i ++) {
			if (srv->bnd[i] != bnd)
				continue;
			memmove(&srv->bnd[i], &srv->bnd[(i + 1)],
			    (sizeof(http_srv_bind_p) * (srv->bind_count - (i + 1))));
			srv->bind_count --;
			break;
		}
	}

	for (i = 0; i < bnd->tptasks_cnt; i ++) {
		tp_task_destroy(bnd->tptasks[i]);
		bnd->tptasks[i] = NULL;
	}
	free(bnd->tptasks);
	bnd->tptasks = NULL;
	bnd->tptasks_cnt = 0;
	hostname_list_deinit(&bnd->hst_name_lst);
	free(bnd);
}

http_srv_p
http_srv_bind_get_srv(http_srv_bind_p bnd) {

	if (NULL == bnd)
		return (NULL);
	return (bnd->srv);
}

void *
http_srv_bind_get_udata(http_srv_bind_p bnd) {

	if (NULL == bnd)
		return (NULL);
	return (bnd->udata);
}

int
http_srv_bind_set_udata(http_srv_bind_p bnd, void *udata) {

	if (NULL == bnd)
		return (EINVAL);
	bnd->udata = udata;
	return (0);
}

int
http_srv_bind_get_addr(http_srv_bind_p bnd, sockaddr_storage_p addr) {

	if (NULL == bnd || NULL == addr)
		return (EINVAL);
	sa_copy(&bnd->s.addr, addr);
	return (0);
}


/* HTTP Client */
http_srv_cli_p
http_srv_cli_alloc(http_srv_bind_p bnd, tpt_p tpt, uintptr_t skt,
    http_srv_cli_ccb_p ccb, void *udata) {
	http_srv_cli_p cli;

	SYSLOGD_EX(LOG_DEBUG, "...");

	cli = calloc(1, sizeof(http_srv_cli_t));
	if (NULL == cli)
		return (cli);
	bnd->srv->stat.connections ++;

	cli->rcv_buf = io_buf_alloc(IO_BUF_FLAGS_STD, bnd->srv->s.rcv_io_buf_init_size);
	if (NULL == cli->rcv_buf)
		goto err_out;
	cli->buf = cli->rcv_buf;
	if (0 != tp_task_create(tpt, skt, tp_task_sr_handler,
	    (TP_TASK_F_CLOSE_ON_DESTROY | TP_TASK_F_CB_AFTER_EVERY_READ),
	    cli, &cli->tptask))
		goto err_out;
	cli->bnd = bnd;
	cli->ccb = (*ccb); /* memcpy */
	cli->udata = udata;

	return (cli);

err_out:
	http_srv_cli_free(cli);
	return (NULL);
}

void
http_srv_cli_free(http_srv_cli_p cli) {

	SYSLOGD_EX(LOG_DEBUG, "...");

	if (NULL == cli)
		return;
	cli->bnd->srv->stat.connections --;
	if (NULL != cli->ccb.on_destroy) { /* Call back handler. */
		cli->ccb.on_destroy(cli, cli->udata, &cli->resp);
	}
	tp_task_destroy(cli->tptask);
	io_buf_free(cli->rcv_buf);
	if (cli->buf != cli->rcv_buf) {
		io_buf_free(cli->buf);
	}
	free(cli);
}

static void
http_srv_cli_next_req(http_srv_cli_p cli) {
	size_t tm = 0;

	if (NULL == cli)
		return;
	/* Move data to buf start. */
	if (NULL != cli->req.data) {
		cli->req.data += cli->req.data_size; /* Move pointer to next request. */
		tm = (size_t)(cli->rcv_buf->used - (size_t)(cli->req.data - cli->rcv_buf->data));
		memmove(cli->rcv_buf->data, cli->req.data, tm);
		/* Restore original value. */
		if (0 != (HTTP_SRV_CLI_FI_NEXT_BYTE_SET & cli->flags_int)) {
			cli->rcv_buf->data[0] = (HTTP_SRV_CLI_FI_NEXT_BYTE_MASK & cli->flags_int);
			cli->flags_int &= ~(HTTP_SRV_CLI_FI_NEXT_BYTE_MASK | HTTP_SRV_CLI_FI_NEXT_BYTE_SET);
		}
	} else {
		debugd_break();
	}
	/* Update used buf size. */
	IO_BUF_BUSY_SIZE_SET(cli->rcv_buf, tm);
	/* Re init client. */
	explicit_bzero(&cli->req, sizeof(http_srv_req_t));
	explicit_bzero(&cli->resp, sizeof(http_srv_resp_t));
}

tp_task_p
http_srv_cli_get_tptask(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (cli->tptask);
}

tp_task_p
http_srv_cli_export_tptask(http_srv_cli_p cli) {
	tp_task_p tptask;

	if (NULL == cli)
		return (NULL);
	tptask = cli->tptask;
	cli->tptask = NULL;
	return (tptask);
}

int
http_srv_cli_import_tptask(http_srv_cli_p cli, tp_task_p tptask,
    tpt_p tpt) {

	if (NULL == cli || NULL == tptask)
		return (EINVAL);

	/* Convert to "ready to read notifier". */
	tp_task_stop(tptask);
	tp_task_udata_set(tptask, cli);
	tp_task_tp_cb_func_set(tptask, tp_task_sr_handler);
	tp_task_flags_set(tptask, (TP_TASK_F_CLOSE_ON_DESTROY | TP_TASK_F_CB_AFTER_EVERY_READ));
	tp_task_tpt_set(tptask, tpt);
	cli->tptask = tptask;

	return (0);
}

io_buf_p
http_srv_cli_get_buf(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (cli->buf);
}

int
http_srv_cli_buf_reset(http_srv_cli_p cli) {

	if (NULL == cli || NULL == cli->buf)
		return (EINVAL);
	IO_BUF_BUSY_SIZE_SET(cli->buf, cli->bnd->srv->s.hdrs_reserve_size);
	return (0);
}

int
http_srv_cli_buf_realloc(http_srv_cli_p cli, int allow_decrease, size_t new_size) {

	if (NULL == cli || NULL == cli->buf)
		return (EINVAL);
	new_size += cli->bnd->srv->s.hdrs_reserve_size;
	if (new_size > cli->buf->size || /* Need more space! */
	    (0 != allow_decrease &&
	     (new_size * 2) < cli->buf->size)) { /* Space too mach. */
		return (io_buf_realloc(&cli->buf, 0, new_size));
	}
	return (0);
}

http_srv_bind_p
http_srv_cli_get_acc(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (cli->bnd);
}

http_srv_p
http_srv_cli_get_srv(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	if (NULL == cli->bnd)
		return (NULL);
	return (cli->bnd->srv);
}

http_srv_req_p
http_srv_cli_get_req(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (&cli->req);
}

http_srv_resp_p
http_srv_cli_get_resp(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (&cli->resp);
}

int
http_srv_cli_ccb_get(http_srv_cli_p cli, http_srv_cli_ccb_p ccb) {

	if (NULL == cli || NULL == ccb)
		return (EINVAL);
	(*ccb) = cli->ccb; /* memcpy */
	return (0);
}

int
http_srv_cli_ccb_set(http_srv_cli_p cli, http_srv_cli_ccb_p ccb) {

	if (NULL == cli || NULL == ccb)
		return (EINVAL);
	cli->ccb = (*ccb); /* memcpy */
	return (0);
}

http_srv_on_req_rcv_cb
http_srv_cli_get_on_req_rcv(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (cli->ccb.on_req_rcv);
}

int
http_srv_cli_set_on_req_rcv(http_srv_cli_p cli, http_srv_on_req_rcv_cb on_req_rcv) {

	if (NULL == cli)
		return (EINVAL);
	cli->ccb.on_req_rcv = on_req_rcv;
	return (0);
}

http_srv_on_resp_snd_cb
http_srv_cli_get_on_rep_snd(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (cli->ccb.on_rep_snd);
}

int
http_srv_cli_set_on_rep_snd(http_srv_cli_p cli, http_srv_on_resp_snd_cb on_rep_snd) {

	if (NULL == cli)
		return (EINVAL);
	cli->ccb.on_rep_snd = on_rep_snd;
	return (0);
}

http_srv_on_destroy_cb
http_srv_cli_get_on_destroy(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (cli->ccb.on_destroy);
}

int
http_srv_cli_set_on_destroy(http_srv_cli_p cli, http_srv_on_destroy_cb on_destroy) {

	if (NULL == cli)
		return (EINVAL);
	cli->ccb.on_destroy = on_destroy;
	return (0);
}

void *
http_srv_cli_get_udata(http_srv_cli_p cli) {

	if (NULL == cli)
		return (NULL);
	return (cli->udata);
}

int
http_srv_cli_set_udata(http_srv_cli_p cli, void *udata) {

	if (NULL == cli)
		return (EINVAL);
	cli->udata = udata;
	return (0);
}

uint32_t
http_srv_cli_get_flags(http_srv_cli_p cli) {

	if (NULL == cli)
		return (0);
	return (cli->flags);
}

int
http_srv_cli_get_addr(http_srv_cli_p cli, sockaddr_storage_p addr) {

	if (NULL == cli || NULL == addr)
		return (EINVAL);
	sa_copy(&cli->addr, addr);
	return (0);
}




/* New connection received. */
static int
http_srv_new_conn_cb(tp_task_p tptask __unused, int error, uintptr_t skt,
    sockaddr_storage_p addr, void *arg) {
	http_srv_cli_p cli;
	http_srv_bind_p bnd;
	http_srv_p srv;
	tpt_p tpt;
	http_srv_cli_ccb_t ccb;
	void *udata;
	char straddr[STR_ADDR_LEN];

	debugd_break_if(NULL == arg);

	bnd = (http_srv_bind_p)arg;
	srv = bnd->srv;
	if (0 != error) {
		close((int)skt);
		srv->stat.errors ++;
		SYSLOG_ERR(LOG_DEBUG, error, "On new conn.");
		return (TP_TASK_CB_CONTINUE);
	}

	/* Default values for new client. */
#ifdef __linux__ /* Linux specific code. */
	/* Linux can balance incomming connections. */
	if (SKT_OPTS_IS_FLAG_ACTIVE(&bnd->s.skt_opts, SO_F_REUSEPORT)) {
		tpt = tp_task_tpt_get(tptask);
	} else {
		tpt = tp_thread_get_rr(srv->tp);
	}
#else
	tpt = tp_thread_get_rr(srv->tp);
#endif
	ccb = srv->ccb; /* memcpy(). */
	udata = NULL;

	sa_addr_port_to_str(addr, straddr, sizeof(straddr), NULL);
	syslog(LOG_DEBUG, "New client: %s (fd: %zu)", straddr, skt);

	/* Call back handler. */
	if (NULL != srv->on_conn) {
		error = srv->on_conn(bnd, srv->udata, skt, addr, &tpt,
		    &ccb, &udata);
		switch (error) {
		case HTTP_SRV_CB_DESTROY:
			close((int)skt);
			return (TP_TASK_CB_CONTINUE);
		case HTTP_SRV_CB_NONE:
			return (TP_TASK_CB_CONTINUE);
		case HTTP_SRV_CB_CONTINUE:
			break; /* OK, continue handling. */
		default:
			debugd_break();
			break;
		}
	}

	cli = http_srv_cli_alloc(bnd, tpt, skt, &ccb, udata);
	if (NULL == cli) {
		if (NULL != ccb.on_destroy) { /* Call back handler. */
			ccb.on_destroy(NULL, udata, NULL);
		}
		close((int)skt);
		srv->stat.errors ++;
		SYSLOG_ERR(LOG_ERR, ENOMEM, "%s: http_srv_cli_alloc().", straddr);
		return (TP_TASK_CB_CONTINUE);
	}
	sa_copy(addr, &cli->addr);
	/* Tune socket. */
	error = skt_opts_apply_ex(skt, SO_F_TCP_ES_CONN_MASK,
	    &bnd->s.skt_opts, addr->ss_family, NULL);
	SYSLOG_ERR(LOG_NOTICE, error,
	    "%s: skt_opts_apply_ex(), this is not fatal.", straddr);
	/* Receive http request. */
	IO_BUF_MARK_TRANSFER_ALL_FREE(cli->rcv_buf);
	/* Shedule data receive / Receive http request. */
	error = tp_task_start_ex(
	    (0 == SKT_OPTS_IS_FLAG_ACTIVE(&bnd->s.skt_opts, SO_F_ACC_FILTER)),
	    cli->tptask, TP_EV_READ, 0, bnd->s.skt_opts.rcv_timeout, 0,
	    cli->rcv_buf, http_srv_recv_done_cb);
	if (0 != error) { /* Error. */
		SYSLOG_ERR(LOG_ERR, error, "tp_task_start_ex(): client ip: %s.", straddr);
		srv->stat.errors ++;
		http_srv_cli_free(cli);
	}
	return (TP_TASK_CB_CONTINUE);
}


/* http request from client is received now, process it. */
static int
http_srv_recv_done_cb(tp_task_p tptask, int error, io_buf_p buf,
    uint32_t eof, size_t transfered_size, void *arg) {
	http_srv_cli_p cli;
	http_srv_bind_p bnd;
	http_srv_p srv;
	char straddr[STR_ADDR_LEN];
	const uint8_t *ptm;
	uint16_t host_port;
	size_t i, tm;
	int action;
	sockaddr_storage_t addr;

	SYSLOGD_EX(LOG_DEBUG, "...");
	debugd_break_if(NULL == arg);
	debugd_break_if(tptask != ((http_srv_cli_p)arg)->tptask);
	debugd_break_if(buf != ((http_srv_cli_p)arg)->rcv_buf);

	cli = (http_srv_cli_p)arg;
	bnd = cli->bnd;
	srv = bnd->srv;
	/* tptask == cli->tptask !!! */
	/* buf == cli->rcv_buf !!! */
	// buf->used = buf->offset;
	action = tp_task_cb_check(buf, eof, transfered_size);
	if (0 != error || TP_TASK_CB_ERROR == action) { /* Fail! :( */
err_out:
		if (0 != error) {
			sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
			SYSLOG_ERR(LOG_ERR, error, "client ip: %s", straddr);
		}
		switch (error) {
		case 0:
			break;
		case ETIMEDOUT:
			srv->stat.timeouts ++;
			break;
		default:
			srv->stat.errors ++;
			break;
		}
		http_srv_cli_free(cli);
		return (TP_TASK_CB_NONE);
	}

	if (0 != (TP_TASK_IOF_F_SYS & eof)) { /* Client call shutdown(, SHUT_WR) and can only receive data. */
		cli->flags |= HTTP_SRV_CLI_F_HALF_CLOSED;
	}

	if (NULL != cli->req.data) { /* Header allready received in prev call, continue receve data. */
		if (TP_TASK_CB_CONTINUE == action &&
		    0 != IO_BUF_TR_SIZE_GET(buf))
			goto continue_recv; /* Continue receive request data. */
		tp_task_flags_add(tptask, TP_TASK_F_CB_AFTER_EVERY_READ);
		goto req_received; /* Have HTTP headers and (all data / OEF). */
	}

	/* Analize HTTP header. */
	ptm = mem_find_cstr(buf->data, buf->used, CRLFCRLF);
	if (NULL == ptm) { /* No HTTP headers end found. */
		if (TP_TASK_CB_CONTINUE != action) { /* Cant receive more, drop. */
drop_cli_without_hdr:
			sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
			syslog(LOG_DEBUG, "No http header, client ip: %s", straddr);
			srv->stat.http_errors ++;
			http_srv_cli_free(cli);
			return (TP_TASK_CB_NONE);
		}
		IO_BUF_MARK_TRANSFER_ALL_FREE(buf);
continue_recv:
		if (0 != IO_BUF_FREE_SIZE(buf) &&
		    IO_BUF_TR_SIZE_GET(buf) <= IO_BUF_FREE_SIZE(buf))
			return (TP_TASK_CB_CONTINUE); /* Continue receive. */
		/* Not enough buf space, try realloc more. */
		tm = (IO_BUF_TR_SIZE_GET(buf) + buf->used);
		if (tm > srv->s.rcv_io_buf_max_size)
			goto drop_cli_without_hdr; /* Request too big. */
		error = io_buf_realloc(&cli->rcv_buf, 0, tm);
		if (0 != error)
			goto err_out;
		buf = cli->rcv_buf;
		tp_task_buf_set(tptask, cli->rcv_buf);
		return (TP_TASK_CB_CONTINUE); /* Continue receive. */
	}
http_hdr_found:
	/* CRLFCRLF - end headers marker found. */
	/* Init request data. */
	cli->req.hdr = buf->data;
	cli->req.hdr_size = (size_t)(ptm - buf->data);
	cli->req.size = (cli->req.hdr_size + 4);
	cli->req.data = (ptm + 4);
	/* Init responce data. */
	cli->resp.p_flags = srv->s.resp_p_flags;

	/* Parse request line. */
	if (0 != http_parse_req_line(cli->req.hdr, cli->req.hdr_size, &cli->req.line)) {
		sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
		syslog(LOG_DEBUG, "http_parse_req_line(): %s", straddr);
		srv->stat.http_errors ++;
		cli->resp.status_code = 400;
stop_and_drop_with_http_err:
		tp_task_stop(tptask);
		cli->resp.p_flags |= HTTP_SRV_RESP_P_F_CONN_CLOSE; /* Force 'connection: close'. */
		cli->resp.p_flags |= HTTP_SRV_RESP_P_F_GEN_ERR_PAGES; /* Generate error page. */
		goto send_error;
	}

	/* Do security cheks. */
	if (0 != http_req_sec_chk(cli->req.hdr, cli->req.hdr_size,
	    cli->req.line.method_code)) {
		/* Something wrong in headers. */
		sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
		syslog(LOG_NOTICE, "http_req_sec_chk() !!! bad http header or request, client ip: %s.", straddr);
		srv->stat.insecure_requests ++;
		cli->resp.status_code = 400;
		goto stop_and_drop_with_http_err;
	}

	/* Request methods additional handling. */
	switch (cli->req.line.method_code) {
	case HTTP_REQ_METHOD_UNKNOWN:
		if (0 == http_hdr_val_get(cli->req.hdr, cli->req.hdr_size,
		    (const uint8_t*)"content-length", 14, &ptm, &tm)) {
			goto handle_content_length;
		}
		/* Assume that no assosiated data with request. */
		cli->req.data_size = 0;
		break;
	case HTTP_REQ_METHOD_GET:
	case HTTP_REQ_METHOD_SUBSCRIBE:
		/* No data in GET and SUBSCRIBE requests. */
		cli->req.data_size = 0;
		break;
	case HTTP_REQ_METHOD_POST:
		if (0 != http_hdr_val_get(cli->req.hdr, cli->req.hdr_size,
		    (const uint8_t*)"content-length", 14, &ptm, &tm)) {
			cli->resp.status_code = 411; /* Length Required. */
			goto stop_and_drop_with_http_err;
		}
handle_content_length:
		cli->req.data_size = ustr2usize(ptm, tm);
		cli->req.size += cli->req.data_size;
		tm = (size_t)(buf->used - (size_t)(cli->req.data - buf->data)); /* Received data size. */
		if (cli->req.data_size <= tm) /* All data received. */
			break;
		if (cli->req.data_size > srv->s.rcv_io_buf_max_size) {
			cli->resp.status_code = 413; /* Request Entity Too Large. */
			goto stop_and_drop_with_http_err;
		}
		/* Need receive nore data. */
		if ((TP_TASK_CB_CONTINUE != action && TP_TASK_CB_NONE != action) ||
		    0 != (HTTP_SRV_CLI_F_HALF_CLOSED & cli->flags)) { /* But we cant! */
			cli->resp.status_code = 400; /* Bad request. */
			goto stop_and_drop_with_http_err;
		}
		tp_task_flags_del(tptask, TP_TASK_F_CB_AFTER_EVERY_READ);
		IO_BUF_TR_SIZE_SET(buf, (cli->req.data_size - tm));
		SYSLOGD_EX(LOG_DEBUG, "tm = %zu, buf->transfer_size = %zu...",
		    tm, IO_BUF_TR_SIZE_GET(buf));
		goto continue_recv;
	}
	
req_received: /* Full request received! */
	tp_task_stop(tptask);
	/* Update stat. */
	if (HTTP_REQ_METHOD__COUNT__ > cli->req.line.method_code) {
		srv->stat.requests[cli->req.line.method_code] ++;
	}
	srv->stat.requests_total ++;

	SYSLOGD_EX(LOG_DEBUG, "req in: size=%zu, req line size = %zu, hdr_size = %zu"
	    "\n==========================================="
	    "\n%.*s"
	    "\n===========================================",
	    buf->used, cli->req.line.line_size, cli->req.hdr_size,
	    (int)cli->req.hdr_size, cli->req.hdr);


	/* Process some headers. */
	/* Process 'connection' header value. */
	if (0 != (HTTP_SRV_REQ_P_F_CONNECTION & srv->s.req_p_flags)) {
		if (0 == http_hdr_val_get(cli->req.hdr, cli->req.hdr_size,
		    (const uint8_t*)"connection", 10, &ptm, &tm)) {
			if (0 == mem_cmpin_cstr("close", ptm, tm)) {
				cli->req.flags |= HTTP_SRV_RD_F_CONN_CLOSE;
			}
		} else if (HTTP_VER_1_1 > cli->req.line.proto_ver) {
			cli->req.flags |= HTTP_SRV_RD_F_CONN_CLOSE;
		}
	}

	/* Process 'host' header value. */
	if (0 == (HTTP_SRV_REQ_P_F_HOST & srv->s.req_p_flags))
		goto skip_host_hdr;
	if (0 != http_hdr_val_get(cli->req.hdr, cli->req.hdr_size,
	    (const uint8_t*)"host", 4, &cli->req.host, &cli->req.host_size)) { /* No "host" hdr. */
		if (HTTP_VER_1_1 > cli->req.line.proto_ver) {
			cli->req.flags |= HTTP_SRV_RD_F_HOST_IS_LOCAL;
		}
		goto skip_host_hdr;
	}
	/* Is 'host' from request line and from header euqual? */
	if (NULL != cli->req.line.host) {
		if (0 != mem_cmpn(cli->req.host, cli->req.host_size,
		    cli->req.line.host, cli->req.line.host_size)) {
			sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
			syslog(LOG_NOTICE, "%s: host in request line: \"%.*s\" "
			    "does not euqual host in headers: \"%.*s\".",
			    straddr,
			    (int)cli->req.line.host_size, cli->req.line.host,
			    (int)cli->req.host_size, cli->req.host);
			srv->stat.insecure_requests ++;
			cli->resp.status_code = 400;
			cli->resp.p_flags |= HTTP_SRV_RESP_P_F_GEN_ERR_PAGES; /* Generate error page. */
			goto send_error;
		}
	}
	if (0 == sa_addr_port_from_str(&addr, (const char*)cli->req.host,
	    cli->req.host_size)) { /* Binary host address. */
		/* Is connection to loopback from ext host? */
		if (0 != sa_addr_is_loopback(&addr) && /* To loopback */
		    0 == sa_addr_is_loopback(&cli->addr)) { /* From net */
conn_from_net_to_loopback:
			sa_addr_port_to_str(&cli->addr, straddr,
			    sizeof(straddr), NULL);
			syslog(LOG_NOTICE, "HACKING ATTEMPT: %s set in host header loopback address.", straddr);
			srv->stat.insecure_requests ++;
			cli->resp.status_code = 403;
			cli->resp.p_flags |= HTTP_SRV_RESP_P_F_GEN_ERR_PAGES; /* Generate error page. */
			goto send_error;
		}
		host_port = sa_port_get(&addr);
		if (0 == host_port) { /* Def http port. */
			host_port = HTTP_PORT;
		}
		if ((0 != (HTTP_SRV_REQ_P_F_HOST_ANY_PORT & srv->s.req_p_flags) ||
		     sa_port_get(&bnd->s.addr) == host_port) &&
		    (0 == hostname_list_check_any(&bnd->hst_name_lst) ||
		     0 == hostname_list_check_any(&srv->hst_name_lst))) {
			cli->req.flags |= HTTP_SRV_RD_F_HOST_IS_LOCAL;
			goto skip_host_hdr;
		}
		arg = NULL; /* Addr info cache. */
		for (i = 0; i < srv->bind_count; i ++) {
			if (srv->bnd[i]->s.addr.ss_family != addr.ss_family ||
			    (0 == (HTTP_SRV_REQ_P_F_HOST_ANY_PORT & srv->s.req_p_flags) &&
			     sa_port_get(&srv->bnd[i]->s.addr) != host_port)) /* not equal port! */
				continue;
			if (0 == sa_addr_is_specified(&srv->bnd[i]->s.addr)) {
				/* Binded to: 0.0.0.0 or [::]. */
				if (0 == is_host_addr_ex(&addr, &arg))
					continue;
			} else { /* Binded to IP addr. */
				if (0 == sa_addr_is_eq(&srv->bnd[i]->s.addr, &addr))
					continue;
			}
			cli->req.flags |= HTTP_SRV_RD_F_HOST_IS_LOCAL;
			break;
		}
		is_host_addr_ex_free(arg);
	} else { /* Text host address. */
		cli->req.flags |= HTTP_SRV_RD_F_HOST_IS_STR;
		ptm = mem_chr(cli->req.host, cli->req.host_size, ':');
		host_port = HTTP_PORT;
		if (NULL == ptm) {
			tm = cli->req.host_size;
		} else {
			ptm ++;
			tm = (size_t)(ptm - cli->req.host);
			if (cli->req.host_size > tm) {
				host_port = ustr2u16(ptm,
				    (cli->req.host_size - tm));
			}
			tm --;
		}
		action = (0 == mem_cmpin_cstr("localhost", cli->req.host, tm));
		/* Is connection to loopback from ext host? */
		if (0 != action &&
		    0 == sa_addr_is_loopback(&cli->addr)) /* from ext host? */
			goto conn_from_net_to_loopback;
		/* Is hostname point to this host? */
		if ((0 != (HTTP_SRV_REQ_P_F_HOST_ANY_PORT & srv->s.req_p_flags) ||
		     sa_port_get(&bnd->s.addr) == host_port) &&
		    (0 != action ||
		     0 == hostname_list_check(&bnd->hst_name_lst, cli->req.host, tm) ||
		     0 == hostname_list_check(&srv->hst_name_lst, cli->req.host, tm))) {
			cli->req.flags |= HTTP_SRV_RD_F_HOST_IS_LOCAL;
		}
	}
skip_host_hdr:

	/* Call client custom on_req_rcv cb. */
	if (NULL != cli->ccb.on_req_rcv) {
		/* Delayed allocation buffer for answer */
		/* cli->buf != cli->rcv_buf!!!: if cb func realloc buf, then rcv_buf became invalid. */
		if (NULL == cli->buf ||
		    cli->buf == cli->rcv_buf) {
			cli->buf = io_buf_alloc(IO_BUF_FLAGS_STD, srv->s.snd_io_buf_init_size);
			if (NULL == cli->buf) { /* Allocate fail, send error. */
				srv->stat.errors ++;
				srv->stat.http_errors --; /* http_srv_snd_err() will increase it.*/
				cli->buf = cli->rcv_buf;
				cli->resp.status_code = 500;
				cli->resp.buf = cli->buf;
				cli->resp.p_flags |= HTTP_SRV_RESP_P_F_CONN_CLOSE; /* Force 'connection: close'. */
				cli->resp.p_flags |= HTTP_SRV_RESP_P_F_GEN_ERR_PAGES; /* Generate error page. */
				goto send_error;
			}
		}
		cli->resp.buf = cli->buf;
		/* Reserve space for HTTP headers. */
		IO_BUF_BUSY_SIZE_SET(cli->buf, srv->s.hdrs_reserve_size);
		/* Update responce data. */
		if (0 != (HTTP_SRV_CLI_F_HALF_CLOSED & cli->flags) ||
		    0 != (HTTP_SRV_RD_F_CONN_CLOSE & cli->req.flags)) {
			cli->resp.p_flags |= HTTP_SRV_RESP_P_F_CONN_CLOSE;
		}
		/* Zeroize end of string, keep original value. */
		if ((cli->req.data + cli->req.data_size) < (buf->data + buf->size)) {
			/* Only if enough buf space. */
			cli->flags_int &= ~HTTP_SRV_CLI_FI_NEXT_BYTE_MASK; /* Ensure that no prev value data stored. */
			cli->flags_int |= (cli->req.data[cli->req.data_size] | HTTP_SRV_CLI_FI_NEXT_BYTE_SET);
			cli->rcv_buf->data[cli->req.size] = 0; /* cli->req.data[cli->req.data_size] = 0; */
		} else {
			cli->flags_int &= ~(HTTP_SRV_CLI_FI_NEXT_BYTE_MASK | HTTP_SRV_CLI_FI_NEXT_BYTE_SET);
		}

		/* Call back handler func. */
		action = cli->ccb.on_req_rcv(cli, cli->udata, &cli->req, &cli->resp);

		/* Handle call back function return code. */
		switch (action) {
		case HTTP_SRV_CB_DESTROY:
			http_srv_cli_free(cli);
			return (TP_TASK_CB_NONE);
		case HTTP_SRV_CB_NONE:
			return (TP_TASK_CB_NONE);
		case HTTP_SRV_CB_CONTINUE:
			break; /* OK, continue handling. */
		default:
			debugd_break();
			break;
		}
	} else { /* Default action. */
		cli->resp.status_code = 404;
		cli->resp.p_flags |= HTTP_SRV_RESP_P_F_CONN_CLOSE; /* Force 'connection: close'. */
		cli->resp.p_flags |= HTTP_SRV_RESP_P_F_GEN_ERR_PAGES; /* Generate error page. */
		goto send_error;
	}

	/* Sending data. */
send_error:
	action = http_srv_send_responce(cli, &ptm);
	if (TP_TASK_CB_CONTINUE == action) /* Next HTTP request headers end found. */
		goto http_hdr_found;

	return (TP_TASK_CB_NONE);
}

static int
http_srv_send_responce(http_srv_cli_p cli, const uint8_t **delimiter) {
	int error;
	uint8_t *ptm;
	char straddr[STR_ADDR_LEN];

	if (NULL == cli)
		return (EINVAL);
	/* Sending data. */
	error = http_srv_snd(cli);
	if (EINPROGRESS == error) /* Send sheduled. */
		return (TP_TASK_CB_NONE);
	/* Data sended. */
	/* Is connection close? */
	if (0 != (HTTP_SRV_CLI_F_HALF_CLOSED & cli->flags) ||
	    0 != (HTTP_SRV_RD_F_CONN_CLOSE & cli->req.flags) ||
	    0 != (HTTP_SRV_RESP_P_F_CONN_CLOSE & cli->resp.p_flags)) {
		http_srv_cli_free(cli); /* Force destroy. */
		return (TP_TASK_CB_NONE);
	}
	/* If sended without error and connection keep-alive, try handle next request. */
	if (0 == error) {
		http_srv_cli_next_req(cli); /* Move data in buffer and do some prepares. */
		/* Analize HTTP header. */
		ptm = mem_find_cstr(cli->rcv_buf->data, cli->rcv_buf->used, CRLFCRLF);
		if (NULL != ptm) { /* HTTP headers end found. */
			if (NULL != delimiter) {
				(*delimiter) = ptm;
			}
			return (TP_TASK_CB_CONTINUE);
		}
		/* Need receive more data. */
		IO_BUF_MARK_TRANSFER_ALL_FREE(cli->rcv_buf);
		tp_task_flags_add(cli->tptask, TP_TASK_F_CB_AFTER_EVERY_READ);
		error = tp_task_restart(cli->tptask);
	}
	if (0 != error) { /* Error. */
		sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
		SYSLOG_ERR(LOG_ERR, error, "http_srv_send_responce: client ip: %s.", straddr);
		cli->bnd->srv->stat.errors ++;
		http_srv_cli_free(cli);
	}
	return (TP_TASK_CB_NONE);
}

int
http_srv_resume_responce(http_srv_cli_p cli) {
	int error;
	char straddr[STR_ADDR_LEN];

	if (NULL == cli)
		return (EINVAL);
	error = http_srv_send_responce(cli, NULL);
	if (TP_TASK_CB_NONE == error)
		return (0);
	error = http_srv_recv_done_cb(cli->tptask, 0, cli->rcv_buf, 0,
	    cli->rcv_buf->used, cli);
	if (TP_TASK_CB_NONE == error)
		return (0);
	/* Need receive more data. */
	IO_BUF_MARK_TRANSFER_ALL_FREE(cli->rcv_buf);
	tp_task_flags_add(cli->tptask, TP_TASK_F_CB_AFTER_EVERY_READ);
	error = tp_task_restart(cli->tptask);
	if (0 != error) { /* Error. */
		sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
		SYSLOG_ERR(LOG_ERR, error, "http_srv_resume_responce: client ip: %s.", straddr);
		cli->bnd->srv->stat.errors ++;
		http_srv_cli_free(cli);
	}
	return (0);
}

int
http_srv_resume_next_request(http_srv_cli_p cli) {
	int error;
	char straddr[STR_ADDR_LEN];

	if (NULL == cli)
		return (EINVAL);
	/* Move data in buffer and do some prepares. */
	http_srv_cli_next_req(cli);
	/* Shedule data receive / Process next. */
	IO_BUF_MARK_TRANSFER_ALL_FREE(cli->rcv_buf);
	tp_task_flags_add(cli->tptask, TP_TASK_F_CB_AFTER_EVERY_READ);
	error = tp_task_start_ex((0 == cli->rcv_buf->used), cli->tptask,
	    TP_EV_READ, 0, cli->bnd->s.skt_opts.rcv_timeout, 0,
	    cli->rcv_buf, http_srv_recv_done_cb);
	if (0 != error) { /* Error. */
		sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
		SYSLOG_ERR(LOG_ERR, error, "http_srv_resume_next_request: client ip: %s.", straddr);
		cli->bnd->srv->stat.errors ++;
		http_srv_cli_free(cli);
	}
	return (0);
}


/* http answer to cli is sended, work done. */
static int
http_srv_snd_done_cb(tp_task_p tptask, int error, io_buf_p buf __unused,
    uint32_t eof, size_t transfered_size __unused, void *arg) {
	int action;
	http_srv_cli_p cli = (http_srv_cli_p)arg;
	char straddr[STR_ADDR_LEN];

	SYSLOGD_EX(LOG_DEBUG, "...");
	debugd_break_if(NULL == arg);
	debugd_break_if(tptask != ((http_srv_cli_p)arg)->tptask);

	if (0 != error) { /* Fail! :( */
		sa_addr_port_to_str(&cli->addr, straddr, sizeof(straddr), NULL);
		SYSLOG_ERR(LOG_ERR, error, "http_srv_snd_done_cb: client ip: %s.", straddr);
		switch (error) {
		case ETIMEDOUT:
			cli->bnd->srv->stat.timeouts ++;
			break;
		default:
			cli->bnd->srv->stat.errors ++;
			break;
		}
		http_srv_cli_free(cli);
		return (TP_TASK_CB_NONE);
	}

	if (0 != eof) { /* Client call shutdown(, SHUT_WR) and can only receive data. */
		cli->flags |= HTTP_SRV_CLI_F_HALF_CLOSED;
	}

	if (NULL != cli->ccb.on_rep_snd) { /* Call back handler. */
		action = cli->ccb.on_rep_snd(cli, cli->udata, &cli->resp);
	} else {
		action = HTTP_SRV_CB_CONTINUE;
	}
	if (0 != (HTTP_SRV_CLI_F_HALF_CLOSED & cli->flags) ||
	    0 != (HTTP_SRV_RD_F_CONN_CLOSE & cli->req.flags) ||
	    0 != (HTTP_SRV_RESP_P_F_CONN_CLOSE & cli->resp.p_flags)) {
		action = HTTP_SRV_CB_DESTROY; /* Force destroy. */
	}

	/* Handle call back function return code. */
	switch (action) {
	case HTTP_SRV_CB_DESTROY:
		http_srv_cli_free(cli);
		return (TP_TASK_CB_NONE);
	case HTTP_SRV_CB_NONE:
		tp_task_stop(cli->tptask);
		return (TP_TASK_CB_NONE);
	case HTTP_SRV_CB_CONTINUE:
		/* OK, continue handling. */
		break;
	default:
		debugd_break();
		break;
	}

	/* Reuse connection. */
	tp_task_stop(cli->tptask);

	/* Move data in buffer and do some prepares. */
	/* Shedule data receive / Process next. */
	http_srv_resume_next_request(cli);

	return (TP_TASK_CB_NONE);
}


/* Offset must pont to data start, size = data offset + data size. */
static int
http_srv_snd(http_srv_cli_p cli) {
	int error, http_err = 0;
	http_srv_p srv;
	http_srv_resp_p resp;
	uint8_t	*wr_pos;
	size_t reason_phrase_size, hdrs_buf_size, hdrs_size, sztm, i;
	ssize_t ios = 0;
	uint64_t data_size;
	char hdrs[1024];
	const char *reason_phrase, *crlf = "\r\n";
	struct iovec iov[IOV_MAX];
	struct msghdr mhdr;

	if (NULL == cli)
		return (EINVAL);
	if (NULL == cli->buf ||
	    cli->buf->used < cli->buf->offset ||
	    HTTP_SRV_RESP_HDS_MAX < cli->resp.hdrs_count) /* Limit custom hdrs count. */
		return (EINVAL);
	srv = cli->bnd->srv;
	resp = &cli->resp;
	data_size = (cli->buf->used - cli->buf->offset);
	if (0 == srv->s.http_server_size) {
		resp->p_flags &= ~HTTP_SRV_RESP_P_F_SERVER; /* Unset flag. */
	}
	if (0 != (HTTP_SRV_CLI_F_HALF_CLOSED & cli->flags)) {
		resp->p_flags |= HTTP_SRV_RESP_P_F_CONN_CLOSE; /* Set flag. */
	}
	if (400 <= resp->status_code &&
	    600 > resp->status_code) {
		srv->stat.http_errors ++;
		if (404 == resp->status_code) {
			srv->stat.unhandled_requests ++;
		}
		http_err ++;
	}

	/* Prepare reason phrase. */
	if (NULL == resp->reason_phrase) { /* Get default responce text. */
		reason_phrase = http_get_err_descr(resp->status_code,
		    &reason_phrase_size);
	} else {
		reason_phrase = resp->reason_phrase;
		reason_phrase_size = resp->reason_phrase_size;
		/* Remove CRLF from tail. */
		while (0 < reason_phrase_size &&
		    ('\r' == reason_phrase[(reason_phrase_size - 1)] ||
		    '\n' == reason_phrase[(reason_phrase_size - 1)])) {
			reason_phrase_size --;
		}
	}

	/* Check buf size: HTTP responce line + standart headers. */
	hdrs_buf_size = (sizeof(hdrs) - 1);
	if (hdrs_buf_size < (256 + reason_phrase_size + 
	    ((0 != (HTTP_SRV_RESP_P_F_SERVER & resp->p_flags)) ? srv->s.http_server_size : 0)))
		return (ENOMEM); /* Not enough space in buf hdrs. */

	/* HTTP header. */

	/* Responce line. */
	memcpy(hdrs, "HTTP/", 5);
	hdrs_size = 5;

	error = u162str(HIWORD(cli->req.line.proto_ver),
	    (hdrs + hdrs_size), (hdrs_buf_size - hdrs_size), &sztm);
	if (0 != error)
		return (error);
	hdrs_size += sztm;

	hdrs[hdrs_size ++] = '.';

	error = u162str(LOWORD(cli->req.line.proto_ver),
	    (hdrs + hdrs_size), (hdrs_buf_size - hdrs_size), &sztm);
	if (0 != error)
		return (error);
	hdrs_size += sztm;

	hdrs[hdrs_size ++] = ' ';

	error = u322str(resp->status_code,
	    (hdrs + hdrs_size), (hdrs_buf_size - hdrs_size), &sztm);
	if (0 != error)
		return (error);
	hdrs_size += sztm;

	hdrs[hdrs_size ++] = ' ';

	memcpy((hdrs + hdrs_size), reason_phrase, reason_phrase_size);
	hdrs_size += reason_phrase_size;
	hdrs[hdrs_size ++] = '\r';
	hdrs[hdrs_size ++] = '\n';

	/* Headers. */
	if (0 != (resp->p_flags & HTTP_SRV_RESP_P_F_SERVER)) {
		memcpy((hdrs + hdrs_size), "Server: ", 8);
		hdrs_size += 8;
		memcpy((hdrs + hdrs_size), srv->s.http_server,
		    srv->s.http_server_size);
		hdrs_size += srv->s.http_server_size;
		hdrs[hdrs_size ++] = '\r';
		hdrs[hdrs_size ++] = '\n';
	}
	if (0 != (resp->p_flags & HTTP_SRV_RESP_P_F_CONTENT_LEN)) {
		memcpy((hdrs + hdrs_size), "Content-Length: ", 16);
		hdrs_size += 16;
		error = u642str(data_size, (hdrs + hdrs_size),
		    (hdrs_buf_size - hdrs_size), &sztm);
		if (0 != error)
			return (error);
		hdrs_size += sztm;
		hdrs[hdrs_size ++] = '\r';
		hdrs[hdrs_size ++] = '\n';
	}
	if (0 != (HTTP_SRV_RESP_P_F_CONN_CLOSE & resp->p_flags)) { /* Conn close. */
		memcpy((hdrs + hdrs_size), "Connection: close\r\n", 19);
		hdrs_size += 19;
	} else if (HTTP_VER_1_1 == cli->req.line.proto_ver &&
	    0 != cli->bnd->s.skt_opts.rcv_timeout) { /* HTTP/1.1 client - keepalive. */
		memcpy((hdrs + hdrs_size),
		    "Connection: keep-alive\r\n"
		    "Keep-Alive: timeout=", 44);
		hdrs_size += 44;
		error = u642str((cli->bnd->s.skt_opts.rcv_timeout / 1000),
		    (hdrs + hdrs_size), (hdrs_buf_size - hdrs_size), &sztm);
		if (0 != error)
			return (error);
		hdrs_size += sztm;
		hdrs[hdrs_size ++] = '\r';
		hdrs[hdrs_size ++] = '\n';
	}

	SYSLOGD_EX(LOG_DEBUG, "\r\n%.*s", (int)hdrs_size, hdrs);

	/* Send data... */
	/* Try "zero copy" send first. */
	memset(&mhdr, 0x00, sizeof(mhdr));
	mhdr.msg_iov = iov;
	mhdr.msg_iovlen ++;
	iov[0].iov_base = hdrs;
	iov[0].iov_len = hdrs_size;

	if (0 != http_err &&
	    0 != (HTTP_SRV_RESP_P_F_GEN_ERR_PAGES & resp->p_flags)) {
		/* HTTP header. */
		iov[mhdr.msg_iovlen].iov_base = MK_RW_PTR(
		    "Content-Type: text/html\r\n"
		    "Pragma: no-cache");
		iov[mhdr.msg_iovlen].iov_len = 41;
		hdrs_size += iov[mhdr.msg_iovlen].iov_len;
		mhdr.msg_iovlen ++;
		if ((cli->buf != cli->rcv_buf && 0 == data_size) ||
		    (cli->buf == cli->rcv_buf &&
		     0 != (HTTP_SRV_RESP_P_F_CONN_CLOSE & resp->p_flags))) {
			IO_BUF_BUSY_SIZE_SET(cli->buf, (hdrs_size + 4));
		}
		/* Data. */
		data_size = cli->buf->used;
		error = io_buf_printf(cli->buf,
		    "<html>\r\n"
		    "	<head><title>%"PRIu32" %.*s</title></head>\r\n"
		    "	<body bgcolor=\"white\">\r\n"
		    "		<center><h1>%"PRIu32" %.*s</h1></center>\r\n"
		    "		<hr><center>"HTTP_LIB_NAME"/"HTTP_LIB_VER"</center>\r\n"
		    "	</body>\r\n"
		    "</html>\r\n",
		    resp->status_code, (int)reason_phrase_size, reason_phrase,
		    resp->status_code, (int)reason_phrase_size, reason_phrase);
		if (0 != error)
			return (error);
		data_size = (cli->buf->used - data_size);
	} else {
		/* Custom headers pre process. */
		for (i = 0; i < resp->hdrs_count; i ++) { /* Add custom headers. */
			if (NULL == resp->hdrs[i].iov_base || 3 > resp->hdrs[i].iov_len)
				continue; /* Skip empty header part. */
			iov[mhdr.msg_iovlen].iov_base = resp->hdrs[i].iov_base;
			iov[mhdr.msg_iovlen].iov_len = resp->hdrs[i].iov_len;
			hdrs_size += iov[mhdr.msg_iovlen].iov_len;
			mhdr.msg_iovlen ++;
			if (0 == memcmp((((uint8_t*)resp->hdrs[i].iov_base) +
			    (resp->hdrs[i].iov_len - 2)), crlf, 2))
				continue; /* No need to add tailing CRLF. */
			iov[mhdr.msg_iovlen].iov_base = MK_RW_PTR(crlf);
			iov[mhdr.msg_iovlen].iov_len = 2;
			hdrs_size += iov[mhdr.msg_iovlen].iov_len;
			mhdr.msg_iovlen ++;
		}
	}
	/* CRLF and recponce body. */
	iov[mhdr.msg_iovlen].iov_base = MK_RW_PTR(crlf);
	iov[mhdr.msg_iovlen].iov_len = 2;
	hdrs_size += 2;
	iov[(1 + mhdr.msg_iovlen)].iov_base = IO_BUF_OFFSET_GET(cli->buf);
	iov[(1 + mhdr.msg_iovlen)].iov_len = data_size;
	mhdr.msg_iovlen += 2;
	//LOGD_EV_FMT("mhdr.msg_iovlen: %zu, data_size: %zu", mhdr.msg_iovlen, data_size);
	/* Try send (write to socket buf).*/
	ios = sendmsg((int)tp_task_ident_get(cli->tptask), &mhdr,
	    (MSG_DONTWAIT | MSG_NOSIGNAL));
	if (-1 == ios)
		return (errno);
	if ((hdrs_size + data_size) == (uint64_t)ios) /* OK, all done. */
		return (0);

	/* Not all data send. */
	if (hdrs_size > (size_t)ios) { /* Not all headers send. */
		/* Copy http headers to buffer before data. */
		hdrs_size -= (size_t)ios;
		IO_BUF_TR_SIZE_SET(cli->buf, (hdrs_size + data_size));
		if (hdrs_size > cli->buf->offset) { /* Worst case :( */
			/* Not enough space before data, move data. */
			/* Check buf free space!!! */
			if ((hdrs_size - cli->buf->offset) >
			    IO_BUF_FREE_SIZE(cli->buf)) {
				error = ENOMEM;
				syslog(LOG_ERR, "Not enough space in socket buffer and in io_buf for HTTP headers, increace skt_snd_buf or/and hdrs_reserve_size.");
				return (error);
			}
			memmove((cli->buf->data + hdrs_size),
			    IO_BUF_OFFSET_GET(cli->buf), data_size);
			cli->buf->used = (hdrs_size + data_size);
			cli->buf->offset = 0;
		} else {
			IO_BUF_OFFSET_DEC(cli->buf, hdrs_size);
		}
		wr_pos = IO_BUF_OFFSET_GET(cli->buf);
		for (i = 0; i < (size_t)(mhdr.msg_iovlen - 1); i ++) {
			if ((size_t)ios > iov[i].iov_len) { /* Skip sended. */
				ios -= iov[i].iov_len;
				continue;
			}
			if (0 != ios) { /* Part of iov buf. */
				iov[i].iov_base = (void*)(((size_t)iov[i].iov_base) + (size_t)ios);
				iov[i].iov_len -= (size_t)ios;
				ios = 0;
			}
			memcpy(wr_pos, iov[i].iov_base, iov[i].iov_len);
			wr_pos += iov[i].iov_len;
		}
	} else { /* All headers sended, send body. */
		IO_BUF_OFFSET_INC(cli->buf, (size_t)((size_t)ios - hdrs_size));
		IO_BUF_TR_SIZE_SET(cli->buf, (cli->buf->used - cli->buf->offset));
	}
	/* Shedule send answer to cli. */
	error = tp_task_start(cli->tptask, TP_EV_WRITE, 0,
	    cli->bnd->s.skt_opts.snd_timeout, 0, cli->buf, http_srv_snd_done_cb);
	if (0 == error) /* No Error, but sheduled. */
		return (EINPROGRESS);
	/* Error. */
	return (error);
}
