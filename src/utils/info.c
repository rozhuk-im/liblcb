/*-
 * Copyright (c) 2013 - 2021 Rozhuk Ivan <rozhuk.im@gmail.com>
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
#include <sys/time.h> /* For getrusage. */
#include <sys/resource.h>
#include <inttypes.h>
#include <stdlib.h> /* malloc, exit */
#include <stdio.h> /* snprintf, fprintf */
#include <unistd.h> /* close, write, sysconf */
#include <string.h> /* memcpy, memmove, memset, strerror... */
#include <time.h>
#include <errno.h>
#ifdef BSD /* BSD specific code. */
#	include <sys/sysctl.h>
#endif /* BSD specific code. */

#include "utils/macro.h"
#include "al/os.h"
#include "utils/mem_utils.h"
#include "utils/info.h"
#include "utils/sys.h"

#ifdef __linux__ /* Linux specific code. */
#define CTL_KERN	0

#define KERN_OSTYPE	0
#define KERN_OSRELEASE	1
#define KERN_NODENAME	2
#define KERN_DOMAINNAME	3
#define KERN_VERSION	4
#endif /* Linux specific code. */


int
sysctl_str_to_buf(int *mib, uint32_t mib_cnt, const char *descr, size_t descr_size,
    char *buf, size_t buf_size, size_t *buf_size_ret) {
	size_t tm;
#ifdef __linux__
	int error, rc;
	const char *l1, *l2;
	char path[1024];
#endif

	if (NULL == descr || descr_size >= buf_size)
		return (EINVAL);

	tm = (buf_size - descr_size);
#ifdef BSD /* BSD specific code. */
	if (0 != sysctl(mib, mib_cnt, (buf + descr_size), &tm, NULL, 0))
		return (errno);
#endif /* BSD specific code. */
#ifdef __linux__ /* Linux specific code. */
	/* sysctl() not implemented, read from: /proc/sys */
	if (2 != mib_cnt)
		return (EINVAL);
	switch (mib[0]) {
	case CTL_KERN:
		l1 = "kernel";
		break;
	default:
		return (EINVAL);
	}

	switch (mib[1]) {
	case KERN_OSTYPE:
		l2 = "ostype";
		break;
	case KERN_OSRELEASE:
		l2 = "osrelease";
		break;
	case KERN_NODENAME:
		l2 = "hostname";
		break;
	case KERN_DOMAINNAME:
		l2 = "domainname";
		break;
	case KERN_VERSION:
		l2 = "version";
		break;
	default:
		return (EINVAL);
	}

	rc = snprintf(path, sizeof(path), "/proc/sys/%s/%s", l1, l2);
	if (IS_SNPRINTF_FAIL(rc, sizeof(path)))
		return (ENOSPC);
	error = read_file_buf(path, (size_t)rc, (buf + descr_size), tm, &tm);
	if (0 != error)
		return (error);
#endif /* Linux specific code. */

	memcpy(buf, descr, descr_size);
	/* Remove CR, LF, TAB, SP, NUL... from end. */
	tm += descr_size;
	while (descr_size < tm && ' ' >= buf[(tm - 1)]) {
		tm --;
	}
	buf[tm] = 0;
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = tm;
	}

	return (0);
}


int
info_get_os_ver(const char *separator, size_t separator_size,
    char *buf, size_t buf_size, size_t *buf_size_ret) {
	int error, mib[4];
	size_t buf_used = 0, tm;

	/* 'OS[sp]version' */
	mib[0] = CTL_KERN;

	mib[1] = KERN_OSTYPE;
	error = sysctl_str_to_buf(mib, 2, NULL, 0, (buf + buf_used),
	    (buf_size - buf_used), &tm);
	if (0 != error)
		return (error);
	buf_used += tm;

	mib[1] = KERN_OSRELEASE;
	error = sysctl_str_to_buf(mib, 2, separator, separator_size,
	    (buf + buf_used), (buf_size - buf_used), &tm);
	if (0 != error)
		return (error);
	buf_used += tm;

	buf[buf_used] = 0;
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = buf_used;
	}

	return (0);
}


int
info_sysinfo(char *buf, size_t buf_size, size_t *buf_size_ret) {
	size_t tm, buf_used = 0;
	int rc, mib[4];
#ifdef BSD /* BSD specific code. */
	size_t itm;
#endif /* BSD specific code. */
#ifdef __linux__ /* Linux specific code. */
	uint64_t tm64;
	uint8_t fbuf[1024], *model = NULL, *cl_rate = NULL, *ptm;
	size_t fbuf_size;
#endif /* Linux specific code. */

	/* Kernel */
	rc = (int)strlcpy((buf + buf_used),
	    "System info", (buf_size - buf_used));
	if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
		goto err_out_snprintf;
	buf_used += (size_t)rc;

	mib[0] = CTL_KERN;

	mib[1] = KERN_OSTYPE;
	if (0 == sysctl_str_to_buf(mib, 2, "\r\nOS: ", 6,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

	mib[1] = KERN_OSRELEASE;
	if (0 == sysctl_str_to_buf(mib, 2, " ", 1,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

#ifdef KERN_HOSTNAME 
	mib[1] = KERN_HOSTNAME;
#else
	mib[1] = KERN_NODENAME; /* linux */
#endif
	if (0 == sysctl_str_to_buf(mib, 2, "\r\nHostname: ", 12,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

#ifdef KERN_NISDOMAINNAME
	mib[1] = KERN_NISDOMAINNAME;
#else
	mib[1] = KERN_DOMAINNAME; /* linux */
#endif
	if (0 == sysctl_str_to_buf(mib, 2, ".", 1,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

	mib[1] = KERN_VERSION;
	if (0 == sysctl_str_to_buf(mib, 2, "\r\nVersion: ", 11,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

	rc = (int)strlcpy((buf + buf_used),
	    "\r\n"
	    "\r\n"
	    "Hardware", (buf_size - buf_used));
	if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
		goto err_out_snprintf;
	buf_used += (size_t)rc;

	/* Hardware */
#ifdef BSD /* BSD specific code. */
	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	if (0 == sysctl_str_to_buf(mib, 2, "\r\nMachine: ", 11,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

	mib[1] = HW_MACHINE_ARCH;
	if (0 == sysctl_str_to_buf(mib, 2, "\r\nArch: ", 8,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

	mib[1] = HW_MODEL;
	if (0 == sysctl_str_to_buf(mib, 2, "\r\nModel: ", 9,
	    (buf + buf_used), (buf_size - buf_used), &tm)) {
		buf_used += tm;
	}

	itm = 0;
	tm = sizeof(itm);
	if (0 == sysctlbyname("hw.clockrate", &itm, &tm, NULL, 0)) {
		rc = snprintf((buf + buf_used), (buf_size - buf_used),
		    "\r\nClockrate: %zu mHz", itm);
		if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
			goto err_out_snprintf;
		buf_used += (size_t)rc;
	}

	mib[1] = HW_NCPU;
	itm = 0;
	tm = sizeof(itm);
	if (0 == sysctl(mib, 2, &itm, &tm, NULL, 0)) {
		rc = snprintf((buf + buf_used), (buf_size - buf_used),
		    "\r\nCPU count: %zu", itm);
		if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
			goto err_out_snprintf;
		buf_used += (size_t)rc;
	}

	mib[1] = HW_PHYSMEM;
	itm = 0;
	tm = sizeof(itm);
	if (0 == sysctl(mib, 2, &itm, &tm, NULL, 0)) {
		rc = snprintf((buf + buf_used), (buf_size - buf_used),
		    "\r\nPhys mem: %zu mb", (itm / (1024 * 1024)));
		if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
			goto err_out_snprintf;
		buf_used += (size_t)rc;
	}
#endif /* BSD specific code. */
#ifdef __linux__ /* Linux specific code. */
	if (0 == read_file_buf("/proc/cpuinfo", 13, fbuf, sizeof(fbuf), &fbuf_size)) {
		model = mem_find_cstr(fbuf, fbuf_size, "model name	:");
		if (NULL != model) {
			model += 13;
			ptm = mem_chr_ptr(model, fbuf, fbuf_size, '\n');
			if (NULL != ptm) {
				(*ptm) = 0;
			}
		}
		cl_rate = mem_find_cstr(fbuf, fbuf_size, "cpu MHz		:");
		if (NULL != cl_rate) {
			cl_rate += 11;
			ptm = mem_chr_ptr(cl_rate, fbuf, fbuf_size, '\n');
			if (NULL != ptm) {
				(*ptm) = 0;
			}
		}
	}
	if (NULL == model) {
		model = (uint8_t*)"";
	}
	if (NULL == cl_rate) {
		cl_rate = (uint8_t*)"";
	}
	tm64 = sysconf(_SC_PHYS_PAGES);
	tm64 *= sysconf(_SC_PAGE_SIZE);
	tm64 /= (1024 * 1024);
	rc = snprintf((buf + buf_used), (buf_size - buf_used),
	    "\r\n"
	    "Model: %s\r\n"
	    "Clockrate: %s\r\n"
	    "CPU count: %li\r\n"
	    "Phys mem: %"PRIu64" mb",
	    model,
	    cl_rate,
	    sysconf(_SC_NPROCESSORS_CONF),
	    tm64);
	if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
		goto err_out_snprintf;
	buf_used += (size_t)rc;
#endif /* Linux specific code. */

	if (NULL != buf_size_ret) {
		(*buf_size_ret) = buf_used;
	}

	return (0);

err_out_snprintf:
	if (0 > rc) /* Error. */
		return (EFAULT);
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = (buf_used + (size_t)rc);
	}
	return (ENOSPC); /* Truncated. */
}


int
info_limits(char *buf, size_t buf_size, size_t *buf_size_ret) {
	int rc;
	size_t i, buf_used = 0;
	struct rlimit rlp;
	const int resource[] = {
		RLIMIT_NOFILE,
		RLIMIT_AS,
		RLIMIT_MEMLOCK,
		RLIMIT_DATA,
		RLIMIT_RSS,
		RLIMIT_STACK,
		RLIMIT_CPU,
		RLIMIT_FSIZE,
		RLIMIT_CORE,
		RLIMIT_NPROC,
#ifdef RLIMIT_SBSIZE
		RLIMIT_SBSIZE,
		RLIMIT_SWAP,
		RLIMIT_NPTS,
#endif
	};
	const char *res_descr[] = {
		"Max open files",
		"Virtual memory max map",
		"mlock max size",
		"Data segment max size",
		"Resident set max size",
		"Stack segment max size",
		"CPU time max",
		"File size max",
		"Core file max size",
		"Processes max count",
		"Socket buffer max size",
		"Swap space max size",
		"Pseudo-terminals max count"
	};

	rc = snprintf((buf + buf_used), (buf_size - buf_used),
	    "Limits\r\n"
	    "CPU count: %li\r\n"
	    "IOV maximum: %li\r\n",
	    sysconf(_SC_NPROCESSORS_ONLN),
	    sysconf(_SC_IOV_MAX));
	if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
		goto err_out_snprintf;
	buf_used += (size_t)rc;

	for (i = 0; i < nitems(resource); i ++) {
		if (0 != getrlimit(resource[i], &rlp))
			continue;

		/* Res name. */
		rc = snprintf((buf + buf_used), (buf_size - buf_used),
		    "%s: ", res_descr[i]);
		if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
			goto err_out_snprintf;
		buf_used += (size_t)rc;

		/* Res current value. */
		if (RLIM_INFINITY == rlp.rlim_cur) {
			rc = (int)strlcpy((buf + buf_used),
			    "infinity / ", (buf_size - buf_used));
		} else {
			rc = snprintf((buf + buf_used), (buf_size - buf_used),
			    "%zu / ", (size_t)rlp.rlim_cur);
		}
		if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
			goto err_out_snprintf;
		buf_used += (size_t)rc;

		/* Res current value. */
		if (RLIM_INFINITY == rlp.rlim_max) {
			rc = (int)strlcpy((buf + buf_used),
			    "infinity\r\n", (buf_size - buf_used));
		} else {
			rc = snprintf((buf + buf_used), (buf_size - buf_used),
			    "%zu\r\n", (size_t)rlp.rlim_max);
		}
		if (IS_SNPRINTF_FAIL(rc, (buf_size - buf_used)))
			goto err_out_snprintf;
		buf_used += (size_t)rc;
	}

	if (NULL != buf_size_ret) {
		(*buf_size_ret) = buf_used;
	}

	return (0);

err_out_snprintf:
	if (0 > rc) /* Error. */
		return (EFAULT);
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = (buf_used + (size_t)rc);
	}
	return (ENOSPC); /* Truncated. */
}


int
info_sysres(info_sysres_p sysres, char *buf, size_t buf_size,
    size_t *buf_size_ret) {
	int rc = 0;
	struct timespec ts;
	struct rusage rusage;
	uint64_t tpd, utime, stime;

	if (NULL == sysres)
		return (EINVAL);

	if (0 != getrusage(RUSAGE_SELF, &rusage) ||
	    0 != clock_gettime(CLOCK_MONOTONIC_FAST, &ts))
		return (errno);
	if (NULL == buf || 0 == buf_size) /* Only init/update internal data. */
		goto upd_int_data;
	tpd = (1000000000 * ((uint64_t)ts.tv_sec - (uint64_t)sysres->upd_time.tv_sec));
	tpd += ((uint64_t)ts.tv_nsec - (uint64_t)sysres->upd_time.tv_nsec);
	if (0 == tpd) { /* Prevent division by zero. */
		tpd ++;
	}
	utime = (1000000 * ((uint64_t)rusage.ru_utime.tv_sec - (uint64_t)sysres->ru_utime.tv_sec));
	utime += ((uint64_t)rusage.ru_utime.tv_usec - (uint64_t)sysres->ru_utime.tv_usec);
	utime = ((utime * 10000000) / tpd);
	stime = (1000000 * ((uint64_t)rusage.ru_stime.tv_sec - (uint64_t)sysres->ru_stime.tv_sec));
	stime += ((uint64_t)rusage.ru_stime.tv_usec - (uint64_t)sysres->ru_stime.tv_usec);
	stime = ((stime * 10000000) / tpd);
	tpd = (utime + stime);
	rc = snprintf(buf, buf_size,
	    "Res usage\r\n"
	    "CPU usage system: %"PRIu64",%02"PRIu64"%%\r\n"
	    "CPU usage user: %"PRIu64",%02"PRIu64"%%\r\n"
	    "CPU usage total: %"PRIu64",%02"PRIu64"%%\r\n"
	    "Max resident set size: %li mb\r\n"
	    "Integral shared text memory size: %li\r\n"
	    "Integral unshared data size: %li\r\n"
	    "Integral unshared stack size: %li\r\n"
	    "Page reclaims: %li\r\n"
	    "Page faults: %li\r\n"
	    "Swaps: %li\r\n"
	    "Block input operations: %li\r\n"
	    "Block output operations: %li\r\n"
	    "IPC messages sent: %li\r\n"
	    "IPC messages received: %li\r\n"
	    "Signals received: %li\r\n"
	    "Voluntary context switches: %li\r\n"
	    "Involuntary context switches: %li\r\n",
	    (stime / 100), (stime % 100),
	    (utime / 100), (utime % 100),
	    (tpd / 100), (tpd % 100),
	    (rusage.ru_maxrss / 1024), rusage.ru_ixrss,
	    rusage.ru_idrss, rusage.ru_isrss,
	    rusage.ru_minflt, rusage.ru_majflt,
	    rusage.ru_nswap, rusage.ru_inblock,
	    rusage.ru_oublock, rusage.ru_msgsnd,
	    rusage.ru_msgrcv, rusage.ru_nsignals,
	    rusage.ru_nvcsw, rusage.ru_nivcsw);

	if (ts.tv_sec >= (INFO_SYSRES_UPD_INTERVAL + sysres->upd_time.tv_sec)) {
upd_int_data: /* Update internal data. */
		memcpy(&sysres->upd_time, &ts, sizeof(ts));
		memcpy(&sysres->ru_utime, &rusage.ru_utime, sizeof(struct timeval));
		memcpy(&sysres->ru_stime, &rusage.ru_stime, sizeof(struct timeval));
	}

	if (0 > rc) /* Error. */
		return (EFAULT);
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = (size_t)rc;
	}
	if (buf_size <= (size_t)rc) /* Truncated. */
		return (ENOSPC);
	return (0);
}
