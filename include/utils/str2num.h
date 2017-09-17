/*-
 * Copyright (c) 2005 - 2017 Rozhuk Ivan <rozhuk.im@gmail.com>
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


#ifndef __STR2NUM_H__
#define __STR2NUM_H__

#ifdef _WINDOWS
	#define uint8_t		unsigned char
	#define uint16_t	WORD
	#define int32_t		LONG
	#define uint32_t	DWORD
	#define int64_t		LONGLONG
	#define uint64_t	DWORDLONG
	#define	size_t		SIZE_T
	#define	ssize_t		SSIZE_T
#else
#	include <inttypes.h>
#endif


#define STR2NUM_SIGN(str, str_len, cur_sign)				\
	for (; 0 != str_len; str_len --, str ++) {			\
		register uint8_t cur_char = (uint8_t)(*str);		\
		if ('-' == cur_char) {					\
			cur_sign = -1;					\
		} else if ('+' == cur_char) {				\
			cur_sign = 1;					\
		} else {						\
			break;						\
		}							\
	}

#define STR2NUM(str, str_len, ret_num)					\
	for (; 0 != str_len; str_len --, str ++) {			\
		register uint8_t cur_char = (((uint8_t)(*str)) - '0');	\
		if (9 < cur_char)					\
			continue;					\
		ret_num *= 10;						\
		ret_num += cur_char;					\
	}

#define STR2UNUM(str, str_len, type)					\
	type ret_num = 0;						\
	if (NULL == str || 0 == str_len)				\
		return (0);						\
	STR2NUM(str, str_len, ret_num);					\
	return (ret_num);

#define STR2SNUM(str, str_len, type)					\
	type ret_num = 0, cur_sign = 1;					\
	if (NULL == str || 0 == str_len)				\
		return (0);						\
	STR2NUM_SIGN(str, str_len, cur_sign);				\
	STR2NUM(str, str_len, ret_num);					\
	ret_num *= cur_sign;						\
	return (ret_num);


static inline size_t
str2usize(const char *str, size_t str_len) {

	STR2UNUM(str, str_len, size_t);
}

static inline size_t
ustr2usize(const uint8_t *str, size_t str_len) {

	STR2UNUM(str, str_len, size_t);
}

static inline uint32_t
str2u32(const char *str, size_t str_len) {

	STR2UNUM(str, str_len, uint32_t);
}

static inline uint32_t
ustr2u32(const uint8_t *str, size_t str_len) {

	STR2UNUM(str, str_len, uint32_t);
}

static inline uint64_t
str2u64(const char *str, size_t str_len) {

	STR2UNUM(str, str_len, uint64_t);
}

static inline uint64_t
ustr2u64(const uint8_t *str, size_t str_len) {

	STR2UNUM(str, str_len, uint64_t);
}


/* Signed. */

static inline ssize_t
str2ssize(const char *str, size_t str_len) {

	STR2SNUM(str, str_len, ssize_t);
}

static inline ssize_t
ustr2ssize(const uint8_t *str, size_t str_len) {

	STR2SNUM(str, str_len, ssize_t);
}

static inline int32_t
str2s32(const char *str, size_t str_len) {

	STR2SNUM(str, str_len, int32_t);
}

static inline int32_t
ustr2s32(const uint8_t *str, size_t str_len) {

	STR2SNUM(str, str_len, int32_t);
}

static inline int64_t
str2s64(const char *str, size_t str_len) {

	STR2SNUM(str, str_len, int64_t);
}

static inline int64_t
ustr2s64(const uint8_t *str, size_t str_len) {

	STR2SNUM(str, str_len, int64_t);
}


#endif /* __STR2NUM_H__ */
