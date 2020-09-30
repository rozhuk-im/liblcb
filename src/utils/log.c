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
#include <stdio.h> /* snprintf, fprintf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strerror... */
#include <unistd.h> /* close, write, sysconf */
#include <stdlib.h> /* malloc, exit */
#include <stdarg.h> /* va_start, va_arg */
#include <time.h>

#include "utils/log.h"
#include "utils/macro.h"


uintptr_t g_log_fd = (uintptr_t)-1;



void
log_write_fd(uintptr_t fd, const char *data, size_t data_size) {

	if ((uintptr_t)-1 == fd || NULL == data || 0 == data_size)
		return;
	(void)write((int)fd, data, data_size);
}

void
log_write_err_fd(uintptr_t fd, const char *fname, int line, int error, const char *descr) {
	char buf[16384], err_descr[256];
	const char *empty = "";
	size_t nsize;
	time_t timel;
	struct tm tml;

	if ((uintptr_t)-1 == fd || 0 == error)
		return;
	if (NULL == descr) {
		descr = empty;
	}
	strerror_r(error, err_descr, sizeof(err_descr));
	timel = time(NULL);
	localtime_r(&timel, &tml);
	if (NULL != fname) {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i] "
		    "%s, line %i: %s error %i: %s\r\n",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec,
		    fname, line, descr, error, err_descr);
	} else {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i]: "
		    "%s error %i: %s\r\n",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec,
		    descr, error, err_descr);
	}
	log_write_fd(fd, buf, nsize);
}

void
log_write_err_fmt_fd(uintptr_t fd, const char *fname, int line, int error,
    const char *fmt, ...) {
	char buf[16384], err_descr[256];
	size_t nsize;
	time_t timel;
	struct tm tml;
	va_list ap;

	if ((uintptr_t)-1 == fd || 0 == error || NULL == fmt)
		return;
	strerror_r(error, err_descr, sizeof(err_descr));
	timel = time(NULL);
	localtime_r(&timel, &tml);
	if (NULL != fname) {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i] "
		    "%s, line %i: error %i: %s ",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec,
		    fname, line, error, err_descr);
	} else {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i]: "
		    "error %i: %s ",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec,
		    error, err_descr);
	}
	va_start(ap, fmt);
	nsize += (size_t)vsnprintf((buf + nsize), ((sizeof(buf) - 4) - nsize), fmt, ap);
	va_end(ap);
	nsize = MIN(nsize, (sizeof(buf) - 4));
	buf[nsize] = 0;
	nsize += (size_t)snprintf((buf + nsize), (sizeof(buf) - nsize), "\r\n");

	log_write_fd(fd, buf, nsize);
}

void
log_write_ev_fd(uintptr_t fd, const char *fname, int line, const char *descr) {
	char buf[16384];
	const char *empty = "";
	size_t nsize;
	time_t timel;
	struct tm tml;

	if ((uintptr_t)-1 == fd)
		return;
	if (NULL == descr) {
		descr = empty;
	}
	timel = time(NULL);
	localtime_r(&timel, &tml);
	if (NULL != fname) {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i] "
		    "%s, line %i: %s\r\n",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec,
		    fname, line, descr);
	} else {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i]: "
		    "%s\r\n",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec, descr);
	}
	log_write_fd(fd, buf, nsize);
}

void
log_write_ev_fmt_fd(uintptr_t fd, const char *fname, int line, const char *fmt, ...) {
	char buf[16384];
	size_t nsize;
	time_t timel;
	struct tm tml;
	va_list ap;

	if ((uintptr_t)-1 == fd || NULL == fmt)
		return;
	timel = time(NULL);
	localtime_r(&timel, &tml);
	if (NULL != fname) {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i] "
		    "%s, line %i: ",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec,
		    fname, line);
	} else {
		nsize = (size_t)snprintf(buf, sizeof(buf), "[%04i-%02i-%02i %02i:%02i:%02i]: ",
		    (tml.tm_year + 1900), (tml.tm_mon + 1), tml.tm_mday,
		    tml.tm_hour, tml.tm_min, tml.tm_sec);
	}
	va_start(ap, fmt);
	nsize += (size_t)vsnprintf((buf + nsize), ((sizeof(buf) - 4) - nsize), fmt, ap);
	va_end(ap);
	nsize = MIN(nsize, (sizeof(buf) - 4));
	buf[nsize] = 0;
	nsize += (size_t)snprintf((buf + nsize), (sizeof(buf) - nsize), "\r\n");

	log_write_fd(fd, buf, nsize);
}
