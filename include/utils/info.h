/*-
 * Copyright (c) 2013 - 2018 Rozhuk Ivan <rozhuk.im@gmail.com>
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

 
#ifndef __INFO_H__
#define __INFO_H__

#include <sys/time.h> /* For getrusage. */
#include <sys/types.h>
#include <inttypes.h>

#ifndef INFO_SYSRES_UPD_INTERVAL
#define INFO_SYSRES_UPD_INTERVAL 2 /* Seconds. */
#endif


int	sysctl_str_to_buf(int *mib, uint32_t mib_cnt,
	    const char *descr, size_t descr_size,
	    uint8_t *buf, size_t buf_size, size_t *buf_size_ret);
int	info_get_os_ver(const char *separator, size_t separator_size,
	    char *buf, size_t buf_size, size_t *buf_size_ret);


typedef struct info_sysres_s {
	struct timespec upd_time; /* Last rusage update time. */
	struct timeval ru_utime; /* user time used */
	struct timeval ru_stime; /* system time used */
} info_sysres_t, *info_sysres_p;


int	info_sysres(info_sysres_p sysres, uint8_t *buf, size_t buf_size,
	    size_t *buf_size_ret);

int	info_limits(uint8_t *buf, size_t buf_size, size_t *buf_size_ret);

int	info_sysinfo(uint8_t *buf, size_t buf_size, size_t *buf_size_ret);


#endif /* __INFO_H__ */
