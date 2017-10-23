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
#include <sys/resource.h>

#include <errno.h>
#include <stdio.h>  /* snprintf, fprintf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strnlen, strerror... */
#include <stdlib.h> /* malloc, exit */

#ifdef BSD /* BSD specific code. */
#	include <sys/rtprio.h>
#endif /* BSD specific code. */

#include "utils/mem_utils.h"
#include "utils/str2num.h"
#include "utils/xml.h"
#include "utils/sys_res_limits_xml.h"


void
sys_res_limits_xml(const uint8_t *buf, size_t buf_size) {
	const uint8_t *data;
	size_t data_size;
	int64_t itm64;
	int itm;
	struct rlimit rlp;

	/* System resource limits. */
	if (NULL == buf || 0 == buf_size)
		return;

	if (0 == xml_get_val_int64_args(buf, buf_size, NULL,
	    &itm64, (const uint8_t*)"maxOpenFiles", NULL)) {
		rlp.rlim_cur = itm64;
		rlp.rlim_max = itm64;
		if (0 != setrlimit(RLIMIT_NOFILE, &rlp)) {
			fprintf(stderr, "setrlimit(RLIMIT_NOFILE) error: %i - %s\n",
			    errno, strerror(errno));
		}
	}
	if (0 == xml_get_val_args(buf, buf_size, NULL, NULL, NULL,
	    &data, &data_size,
	    (const uint8_t*)"maxCoreFileSize", NULL) &&
	    0 < data_size) {
		if (0 == mem_cmpin_cstr("unlimited", data, data_size)) {
			rlp.rlim_cur = RLIM_INFINITY;
		} else {
			rlp.rlim_cur = (ustr2s64(data, data_size) * 1024); /* in kb */
		}
		rlp.rlim_max = rlp.rlim_cur;
		if (0 != setrlimit(RLIMIT_CORE, &rlp)) {
			fprintf(stderr, "setrlimit(RLIMIT_CORE) error: %i - %s\n",
			    errno, strerror(errno));
		}
	}
	if (0 == xml_get_val_args(buf, buf_size, NULL, NULL, NULL,
	    &data, &data_size,
	    (const uint8_t*)"maxMemLock", NULL) &&
	    0 < data_size) {
		if (0 == mem_cmpin_cstr("unlimited", data, data_size)) {
			rlp.rlim_cur = RLIM_INFINITY;
		} else {
			rlp.rlim_cur = (ustr2s64(data, data_size) * 1024); /* in kb */
		}
		rlp.rlim_max = rlp.rlim_cur;
		if (0 != setrlimit(RLIMIT_MEMLOCK, &rlp)) {
			fprintf(stderr, "setrlimit(RLIMIT_MEMLOCK) error: %i - %s\n",
			    errno, strerror(errno));
		}
	}
	if (0 == xml_get_val_int32_args(buf, buf_size, NULL,
	    &itm, (const uint8_t*)"processPriority", NULL)) {
		if (0 != setpriority(PRIO_PROCESS, 0, itm)) {
			fprintf(stderr, "setpriority() error: %i - %s\n",
			    errno, strerror(errno));
		}
	}
	if (0 == xml_get_val_int32_args(buf, buf_size, NULL,
	    &itm, (const uint8_t*)"processPriority2", NULL)) {
#ifdef BSD /* BSD specific code. */
		struct rtprio rtp;
		rtp.type = RTP_PRIO_REALTIME;
		rtp.prio = (u_short)itm;
		if (0 != rtprio(RTP_SET, 0, &rtp)) {
			fprintf(stderr, "rtprio() error: %i - %s\n",
			    errno, strerror(errno));
		}
#endif /* BSD specific code. */
	}
}
