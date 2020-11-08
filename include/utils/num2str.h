/*
 * Copyright (c) 2005 - 2020 Rozhuk Ivan <rozhuk.im@gmail.com>
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


#ifndef __NUM2STR_H__
#define __NUM2STR_H__

#include <sys/types.h>
#include <inttypes.h>


#define POW10LST_COUNT		19
static const uint64_t pow10lst[POW10LST_COUNT] = {
	10ull,
	100ull,
	1000ull,
	10000ull,
	100000ull,
	1000000ull,
	10000000ull,
	100000000ull,
	1000000000ull,
	10000000000ull,
	100000000000ull,
	1000000000000ull,
	10000000000000ull,
	100000000000000ull,
	1000000000000000ull,
	10000000000000000ull,
	100000000000000000ull,
	1000000000000000000ull,
	10000000000000000000ull
};


#define UNUM2STR(__num, __type, __buf, __size)				\
	size_t __len;							\
	if (NULL == (__buf) || 0 == (__size))				\
		return (0);						\
	for (__len = 0;							\
	     __len < POW10LST_COUNT && ((uint64_t)(__num)) > pow10lst[__len]; \
	     __len ++)							\
		;							\
	if ((__len + 1) >= (__size))					\
		return ((__len + 2));					\
	(__buf) += __len;						\
	(__buf)[1] = 0;							\
	do {								\
		(*(__buf) --) = (__type)('0' + ((__num) % 10));		\
		(__num) /= 10;						\
	} while ((__num));						\
	return (__len);

#define SNUM2STR(__num, __type, __buf, __size)				\
	size_t __len, __neg = 0;					\
	if (NULL == (__buf) || 0 == (__size))				\
		return (0);						\
	if (0 > (__num)) {						\
		(__num) = - (__num);					\
		__neg = 1;						\
	}								\
	for (__len = 0;							\
	     __len < POW10LST_COUNT && ((uint64_t)(__num)) > pow10lst[__len]; \
	     __len ++)							\
		;							\
	__len += __neg;							\
	if ((__len + 1) >= (__size))					\
		return ((__len + 2));					\
	if (__neg) {							\
		(*(__buf)) = '-';					\
	}								\
	(__buf) += __len;						\
	(__buf)[1] = 0;							\
	do {								\
		(*(__buf) --) = (__type)('0' + ((__num) % 10));		\
		(__num) /= 10;						\
	} while ((__num));						\
	return (__len);


static inline size_t
usize2str(size_t num, char *buf, size_t buf_size) {

	UNUM2STR(num, char, buf, buf_size);
}

static inline size_t
usize2ustr(size_t num, uint8_t *buf, size_t buf_size) {

	UNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
u82str(uint8_t num, char *buf, size_t buf_size) {

	UNUM2STR(num, char, buf, buf_size);
}

static inline size_t
u82ustr(uint8_t num, uint8_t *buf, size_t buf_size) {

	UNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
u162str(uint16_t num, char *buf, size_t buf_size) {

	UNUM2STR(num, char, buf, buf_size);
}

static inline size_t
u162ustr(uint16_t num, uint8_t *buf, size_t buf_size) {

	UNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
u322str(uint32_t num, char *buf, size_t buf_size) {

	UNUM2STR(num, char, buf, buf_size);
}

static inline size_t
u322ustr(uint32_t num, uint8_t *buf, size_t buf_size) {

	UNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
u642str(uint64_t num, char *buf, size_t buf_size) {

	UNUM2STR(num, char, buf, buf_size);
}

static inline size_t
u642ustr(uint64_t num, uint8_t *buf, size_t buf_size) {

	UNUM2STR(num, uint8_t, buf, buf_size);
}


/* Signed. */

static inline ssize_t
ssize2str(ssize_t num, char *buf, size_t buf_size) {

	SNUM2STR(num, char, buf, buf_size);
}

static inline ssize_t
ssize2ustr(ssize_t num, uint8_t *buf, size_t buf_size) {

	SNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
s82str(int8_t num, char *buf, size_t buf_size) {

	SNUM2STR(num, char, buf, buf_size);
}

static inline size_t
s82ustr(int8_t num, uint8_t *buf, size_t buf_size) {

	SNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
s162str(int16_t num, char *buf, size_t buf_size) {

	SNUM2STR(num, char, buf, buf_size);
}

static inline size_t
s162ustr(int16_t num, uint8_t *buf, size_t buf_size) {

	SNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
s322str(int32_t num, char *buf, size_t buf_size) {

	SNUM2STR(num, char, buf, buf_size);
}

static inline size_t
s322ustr(int32_t num, uint8_t *buf, size_t buf_size) {

	SNUM2STR(num, uint8_t, buf, buf_size);
}

static inline size_t
s642str(int64_t num, char *buf, size_t buf_size) {

	SNUM2STR(num, char, buf, buf_size);
}

static inline size_t
s642ustr(int64_t num, uint8_t *buf, size_t buf_size) {

	SNUM2STR(num, uint8_t, buf, buf_size);
}


#endif /* __NUM2STR_H__ */
