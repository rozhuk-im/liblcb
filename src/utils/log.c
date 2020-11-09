/*-
 * Copyright (c) 2011 - 2020 Rozhuk Ivan <rozhuk.im@gmail.com>
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
#include <stdio.h> /* snprintf, fprintf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strerror... */
#include <unistd.h> /* close, write, sysconf */
#include <stdlib.h> /* malloc, exit */
#include <stdarg.h> /* va_start, va_arg */
#include <time.h>
#include <pthread.h>

#include "utils/log.h"
#include "utils/macro.h"


uintptr_t g_log_fd = (uintptr_t)-1;

static pthread_mutex_t log_mutex;
static volatile size_t log_mutex_initialized = 0;


static inline void
log_mutex_init(void) {
	
	if (0 != log_mutex_initialized)
		return;
	log_mutex_initialized ++;
	pthread_mutex_init(&log_mutex, NULL);
	//pthread_mutex_destroy(&log_mutex);
}

static int
log_write_hdr_fd(uintptr_t fd, const char *fname, int line) {
	int rc;
	char buf[1024];
	size_t buf_used;
	time_t _time;
	struct tm _tm;

	if ((uintptr_t)-1 == fd)
		return (EINVAL);

	/* Time. */
	_time = time(NULL);
	localtime_r(&_time, &_tm);
	rc = snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i]",
	    (_tm.tm_year + 1900), (_tm.tm_mon + 1), _tm.tm_mday,
	    _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
	if (IS_SNPRINTF_FAIL(rc, sizeof(buf)))
		return (ENOSPC);
	buf_used = (size_t)rc;

	/* Source file name and line num. */
	if (NULL != fname) {
		rc = snprintf((buf + buf_used), (sizeof(buf) - buf_used),
		    " %s, line %i: ",
		    fname, line);
		if (IS_SNPRINTF_FAIL(rc, (sizeof(buf) - buf_used)))
			return (ENOSPC);
		buf_used += (size_t)rc;
	} else {
		buf[buf_used ++] = ':';
		buf[buf_used ++] = ' ';
	}

	log_write_fd(fd, buf, buf_used);

	return (0);
}


void
log_write_fd(uintptr_t fd, const char *data, size_t data_size) {

	if ((uintptr_t)-1 == fd || NULL == data || 0 == data_size)
		return;
	write((int)fd, data, data_size);
}

void
log_write_err_fd(uintptr_t fd, const char *fname, int line, int error,
    const char *descr) {
	int rc;
	char err_descr[64], err_str[128];
	size_t err_str_size;

	if ((uintptr_t)-1 == fd || 0 == error)
		return;

	strerror_r(error, err_descr, sizeof(err_descr));
	rc = snprintf(err_str, sizeof(err_str), " - error %i: %s\r\n",
	    error, err_descr);
	if (0 > rc)
		return;
	err_str_size = (size_t)rc;

	log_mutex_init();
	pthread_mutex_lock(&log_mutex);
	log_write_hdr_fd(fd, fname, line);
	if (NULL != descr) {
		log_write_fd(fd, descr, strlen(descr));
	}
	log_write_fd(fd, err_str, err_str_size);
	pthread_mutex_unlock(&log_mutex);
}

void
log_write_err_fmt_fd(uintptr_t fd, const char *fname, int line, int error,
    const char *fmt, ...) {
	int rc;
	char buf[16384], err_descr[64], err_str[128];
	size_t buf_used, err_str_size;
	va_list ap;

	if ((uintptr_t)-1 == fd || 0 == error || NULL == fmt)
		return;

	strerror_r(error, err_descr, sizeof(err_descr));
	rc = snprintf(err_str, sizeof(err_str), " - error %i: %s\r\n",
	    error, err_descr);
	if (0 > rc)
		return;
	err_str_size = (size_t)rc;

	va_start(ap, fmt);
	rc = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (0 > rc)
		return;
	buf_used = MIN((size_t)rc, (sizeof(buf) - 1));

	log_mutex_init();
	pthread_mutex_lock(&log_mutex);
	log_write_hdr_fd(fd, fname, line);
	log_write_fd(fd, buf, buf_used);
	log_write_fd(fd, err_str, err_str_size);
	pthread_mutex_unlock(&log_mutex);
}

void
log_write_ev_fd(uintptr_t fd, const char *fname, int line, const char *descr) {

	if ((uintptr_t)-1 == fd)
		return;

	log_mutex_init();
	pthread_mutex_lock(&log_mutex);
	log_write_hdr_fd(fd, fname, line);
	if (NULL != descr) {
		log_write_fd(fd, descr, strlen(descr));
	}
	log_write_fd(fd, "\r\n", 2);
	pthread_mutex_unlock(&log_mutex);
}

void
log_write_ev_fmt_fd(uintptr_t fd, const char *fname, int line,
    const char *fmt, ...) {
	int rc;
	char buf[16384];
	size_t buf_used;
	va_list ap;

	if ((uintptr_t)-1 == fd || NULL == fmt)
		return;

	va_start(ap, fmt);
	rc = vsnprintf(buf, (sizeof(buf) - 4), fmt, ap);
	va_end(ap);
	if (0 > rc)
		return;
	buf_used = MIN((size_t)rc, (sizeof(buf) - 4));
	buf[buf_used ++] = '\r';
	buf[buf_used ++] = '\n';

	log_mutex_init();
	pthread_mutex_lock(&log_mutex);
	log_write_hdr_fd(fd, fname, line);
	log_write_fd(fd, buf, buf_used);
	pthread_mutex_unlock(&log_mutex);
}
