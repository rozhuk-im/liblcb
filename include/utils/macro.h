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


#ifndef __MACRO_HELPERS_H__
#define __MACRO_HELPERS_H__


#include <sys/param.h>
#include <sys/types.h>
#include <inttypes.h>
#include <pthread.h>
#include <syslog.h>
#ifdef _MSC_VER
#	include <intrin.h>
#else
#	include <signal.h>
#endif

#ifndef IOV_MAX
#	include <limits.h>
#	include <stdio.h>
#	include <bits/xopen_lim.h>
#endif


#ifndef SIZEOF /* nitems() */
#	define SIZEOF(__val)	nitems((__val))
#endif

#ifndef ALIGNEX
#	define ALIGNEX(__size, __align_size)				\
		(((__size) + ((size_t)(__align_size)) - 1) & ~(((size_t)(__align_size)) - 1))
#	define ALIGNEX_PTR(__ptr, __align_size)				\
		((((char*)(__ptr)) + ((size_t)(__align_size)) - 1) & ~(((size_t)(__align_size)) - 1))
#endif

#ifndef limit_val
#	define limit_val(__val, __min, __max)				\
		(((__val) > (__max)) ? (__max) : (((__val) < (__min)) ? (__min) : (__val)) )
#endif

#ifndef MAKEDWORD
#	define MAKEDWORD(__lo, __hi)					\
		((((uint32_t)(__lo)) & 0xffff) | ((((uint32_t)(__hi)) & 0xffff) << 16))
#endif

#ifndef LOWORD
#	define LOWORD(__val)	((uint16_t)(((uint32_t)(__val)) & 0xffff))
#endif

#ifndef HIWORD
#	define HIWORD(__val)	((uint16_t)((((uint32_t)(__val)) >> 16) & 0xffff))
#endif


#define UINT32_BIT(__bit)		(((uint32_t)1) << (__bit))
#define UINT32_BIT_SET(__mask, __bit)	(__mask) |= UINT32_BIT((__bit))
#define UINT32_BIT_IS_SET(__mask, __bit) (0 != ((__mask) & UINT32_BIT((__bit))))

#define UINT64_BIT(__bit)		(((uint64_t)1) << (__bit))
#define UINT64_BIT_SET(__mask, __bit)	(__mask) |= UINT64_BIT((__bit))
#define UINT64_BIT_IS_SET(__mask, __bit) (0 != ((__mask) & UINT64_BIT((__bit))))


#define TIMESPEC_TO_MS(__ts)						\
    ((((uint64_t)(__ts)->tv_sec) * 1000) + (((uint64_t)(__ts)->tv_nsec) / 1000000))


#ifndef MK_RW_PTR
#	define MK_RW_PTR(__ptr)	((void*)(size_t)(__ptr))
#endif


#define IS_SNPRINTF_FAIL(__rc, __buf_size)				\
    (0 > (__rc) || (__buf_size) <= (size_t)(__rc))


#define is_space(__c)		(' ' == (__c) || ('\t' <= (__c) && '\r' >= (__c)))

#define IS_NAME_DOTS(__name)	('.' == (__name)[0] &&			\
				 (0 == (__name)[1] || 			\
				  ('.' == (__name)[1] && 0 == (__name)[2])))


#ifndef fieldsetof /* sizeof struct field */
#	define fieldsetof(__type, __field) ((size_t)sizeof(((__type*)0)->__field))
#endif

#define MEMCPY_STRUCT_FIELD(__dst, __sdata, __stype, __sfield)		\
	memcpy((__dst),							\
	    (((const char*)(__sdata)) + offsetof(__stype, __sfield)),	\
	    fieldsetof(__stype, __sfield))


#ifndef MTX_S
#	define MTX_S			pthread_mutex_t
#	define MTX_INIT(__mutex) do {					\
		pthread_mutexattr_t attr;				\
		pthread_mutexattr_init(&attr);				\
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
		pthread_mutex_init((__mutex), &attr);			\
		pthread_mutexattr_destroy(&attr);			\
	} while (0)
#	define MTX_DESTROY(__mutex)	pthread_mutex_destroy((__mutex))
#	define MTX_LOCK(__mutex)	pthread_mutex_lock((__mutex))
#	define MTX_TRYLOCK(__mutex)	pthread_mutex_trylock((__mutex))
#	define MTX_UNLOCK(__mutex)	pthread_mutex_unlock((__mutex))
#endif


#ifndef COND_VAR_S
#	define COND_VAR_S		pthread_cond_t
#	define COND_VAR_INIT(__cond, __shared) do {			\
		pthread_condattr_t attr;				\
		pthread_condattr_init(&attr);				\
		pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);	\
		pthread_condattr_setpshared(&attr,			\
		    ((__shared) ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE)); \
		pthread_cond_init((__cond), &attr);			\
		pthread_condattr_destroy(&attr);			\
	} while (0)
#	define COND_VAR_DESTROY(__cond)	pthread_cond_destroy((__cond))
#	define COND_VAR_BCAST(__cond)	pthread_cond_broadcast((__cond))
#	define COND_VAR_SIGNAL(__cond)	pthread_cond_signal((__cond))
#	define COND_VAR_WAIT(__cond, __mutex) pthread_cond_wait((__cond), (__mutex))
#	define COND_VAR_WAITT(__cond, __mutex, __time) pthread_cond_timedwait((__cond), (__mutex), (__time))
#endif



__attribute__((gnu_inline, always_inline))
static inline void
debug_break(void) {
#ifdef _MSC_VER
	__debugbreak();
#elif 0
	__builtin_trap();
#else
	raise(SIGTRAP);
#endif
}

#define debug_break_if(__a)	if ((__a)) debug_break()

#ifdef DEBUG
#	define debugd_break	debug_break
#	define debugd_break_if	debug_break_if
#else
#	define debugd_break()
#	define debugd_break_if(__a)
#endif


#define SYSLOG_EX(_prio, _fmt, args...)					\
	syslog((_prio), "%s:%i: %s(): "_fmt,				\
	    __FILE__, __LINE__, __FUNCTION__, ##args)

#define SYSLOG_ERR(_prio, _error, _fmt, args...) do {			\
	if (0 == (_error))						\
		break;							\
	errno = (_error);						\
	syslog((_prio), _fmt" Error %i: %m", ##args, (_error));	\
} while (0)

#define SYSLOG_ERR_EX(_prio, _error, _fmt, args...) do {		\
	if (0 == (_error))						\
		break;							\
	errno = (_error);						\
	syslog((_prio), "%s:%i: %s(): "_fmt" Error %i: %m",		\
	    __FILE__, __LINE__, __FUNCTION__, ##args, (_error));	\
} while (0)


#ifdef DEBUG
#	define SYSLOGD		syslog
#	define SYSLOGD_EX	SYSLOG_EX
#	define SYSLOGD_ERR	SYSLOG_ERR
#	define SYSLOGD_ERR_EX	SYSLOG_ERR_EX
#else
#	define SYSLOGD(_prio, _fmt, args...)
#	define SYSLOGD_EX(_prio, _fmt, args...)
#	define SYSLOGD_ERR(_prio, _error, _fmt, args...)
#	define SYSLOGD_ERR_EX(_prio, _error, _fmt, args...)
#endif


#endif /* __MACRO_HELPERS_H__ */
