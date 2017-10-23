/*-
 * Copyright (c) 2015 - 2017 Rozhuk Ivan <rozhuk.im@gmail.com>
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
#include <errno.h>
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strerror... */
#include <stdlib.h> /* malloc, exit */
#include <stdio.h> /* snprintf, fprintf */

#include "utils/mem_utils.h"
#include "utils/str2num.h"
#include "utils/helpers.h"
#include "utils/ini.h"

#define INI_CFG_LINE_ALLOC_PADDING	16


typedef struct ini_line_s {
	uint8_t 	*data;
	size_t		data_size;
	size_t		data_allocated_size;
	uint32_t	type;
	uint8_t 	*name;
	size_t		name_size;
	uint8_t 	*val;
	size_t		val_size;
} ini_line_t, *ini_line_p;
#define INI_LINE_TYPE_EMPTY_LINE	0
#define INI_LINE_TYPE_INVALID		1
#define INI_LINE_TYPE_COMMENT		2
#define INI_LINE_TYPE_SECTION		3
#define INI_LINE_TYPE_VALUE		4


typedef struct ini_s {
	ini_line_p 	*lines;
	size_t		lines_count;
} ini_t;


static ini_line_p
ini_line_alloc__int(size_t size) {
	size_t tm;
	ini_line_p line;

	/* Alloc and store data from line. */
	tm = (sizeof(ini_line_t) + size + INI_CFG_LINE_ALLOC_PADDING);
	line = zalloc(tm);
	if (NULL == line)
		return (NULL);
	line->data = (uint8_t*)(line + 1);
	line->data_size = size;
	line->data_allocated_size = (tm - sizeof(ini_line_t));

	return (line);
}

int
ini_create(ini_p *ini_ret) {
	ini_p ini;

	if (NULL == ini_ret)
		return (EINVAL);

	ini = zalloc(sizeof(ini_t));
	if (NULL == ini) {
		(*ini_ret) = NULL;
		return (errno);
	}
	(*ini_ret) = ini;

	return (0);
}

void
ini_destroy(ini_p ini) {
	size_t i;

	if (NULL == ini)
		return;
	if (NULL != ini->lines) {
		for (i = 0; i < ini->lines_count; i ++) {
			if (NULL == ini->lines[i])
				continue;
			free(ini->lines[i]);
		}
		free(ini->lines);
	}
	free(ini);
}


int
ini_buf_parse(ini_p ini, const uint8_t *buf, size_t buf_size) {
	int error = 0;
	uint8_t *ptr;
	const uint8_t *pline;
	size_t line_size;
	ini_line_p line, *lines_new;

	if (NULL == ini || NULL == buf)
		return (EINVAL);
	if (0 == buf_size)
		return (0);

	pline = NULL;
	line_size = 0;
	while (0 == buf_get_next_line(buf, buf_size, pline, line_size,
	    &pline, &line_size)) {
		/* Alloc and store data from line. */
		line = ini_line_alloc__int(line_size);
		if (NULL == line) {
			error = errno;
			goto err_out;
		}
		memcpy(line->data, pline, line_size);
		if (0 == line_size) {
			line->type = INI_LINE_TYPE_EMPTY_LINE;
		} else {
			switch (line->data[0]) {
			case ';':
			case '#':
				line->type = INI_LINE_TYPE_COMMENT;
				break;
			case '[':
				ptr = mem_rchr(line->data,
				    line->data_size, ']');
				if (NULL == ptr) { /* Bad format. */
					line->type = INI_LINE_TYPE_INVALID;
					break;
				}
				line->type = INI_LINE_TYPE_SECTION;
				line->name = (line->data + 1);
				line->name_size = (size_t)(ptr - line->name);
				break;
			default:
				ptr = mem_chr(line->data,
				    line->data_size, '=');
				if (NULL == ptr) { /* Bad format. */
					line->type = INI_LINE_TYPE_INVALID;
					break;
				}
				line->type = INI_LINE_TYPE_VALUE;
				line->name = line->data;
				line->name_size = (size_t)(ptr - line->name);
				line->val = (ptr + 1);
				line->val_size = (line->data_size -
				    (size_t)(line->val - line->data));
				break;
			}
		}
		/* Add line to array. */
		lines_new = reallocarray(ini->lines,
		    (ini->lines_count + 1), sizeof(ini_line_p));
		if (NULL == lines_new) {
			error = errno;
			free(line);
			goto err_out;
		}
		ini->lines = lines_new;
		ini->lines[ini->lines_count] = line;
		ini->lines_count ++;
	}
err_out:

	return (error);
}

int
ini_buf_calc_size(ini_p ini, size_t *file_size) {
	size_t i, tm;

	if (NULL == ini || NULL == file_size)
		return (EINVAL);

	/* Summ lines. */
	for (i = 0, tm = 0; i < ini->lines_count; i ++) {
		if (NULL == ini->lines[i])
			continue;
		tm += (ini->lines[i]->data_size + 2);
	}
	(*file_size) = tm;

	return (0);
}

int
ini_buf_gen(ini_p ini, uint8_t *buf, size_t buf_size,
    size_t *buf_size_ret) {
	int error = 0;
	size_t i, off, tm;

	if (NULL == ini || NULL == buf || 0 == buf_size ||
	    NULL == buf_size_ret)
		return (EINVAL);

	/* Write lines. */
	for (i = 0, off = 0; i < ini->lines_count; i ++) {
		if (NULL == ini->lines[i])
			continue;
		tm = (ini->lines[i]->data_size + 2);
		if ((off + tm) > buf_size) {
			error = -1;
			break;
		}
		memcpy((buf + off), ini->lines[i]->data,
		    ini->lines[i]->data_size);
		memcpy((buf + off + ini->lines[i]->data_size), "\r\n", 2);
		off += tm;
	}
	(*buf_size_ret) = off;

	return (error);
}


int
ini_sect_enum(ini_p ini, size_t *sect_off, const uint8_t **sect_name,
    size_t *sect_name_size) {
	size_t i;

	if (NULL == ini || NULL == sect_off)
		return (EINVAL);
	if (NULL == ini->lines)
		return (ENOENT);

	if ((*sect_off) > ini->lines_count) {
		(*sect_off) = 0;
	}
	/* Look for section. */
	for (i = (*sect_off); i < ini->lines_count; i ++) {
		if (NULL == ini->lines[i])
			continue;
		if (INI_LINE_TYPE_SECTION != ini->lines[i]->type)
			continue;
		/* Found! */
		(*sect_off) = i;
		if (NULL != sect_name) {
			(*sect_name) = ini->lines[i]->name;
		}
		if (NULL != sect_name_size) {
			(*sect_name_size) = ini->lines[i]->name_size;
		}
		return (0);
	}

	return (ENOENT);
}

size_t
ini_sect_find(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size) {
	size_t i = 0, name_size;
	const uint8_t *name;

	if (NULL == ini || (NULL != sect_name && 0 == sect_name_size))
		return ((size_t)~0);
	if (NULL == ini->lines)
		return ((size_t)~0);

	/* Look for section. */
	while (0 == ini_sect_enum(ini, &i, &name, &name_size)) {
		if (0 == mem_cmpn(name, name_size, sect_name, sect_name_size))
			return (i); /* Found! */
		i ++;
	}

	return ((size_t)~0);
}

size_t
ini_sect_findi(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size) {
	size_t i = 0, name_size;
	const uint8_t *name;

	if (NULL == ini || (NULL != sect_name && 0 == sect_name_size))
		return ((size_t)~0);
	if (NULL == ini->lines)
		return ((size_t)~0);

	/* Look for section. */
	while (0 == ini_sect_enum(ini, &i, &name, &name_size)) {
		if (0 == mem_cmpin(name, name_size, sect_name, sect_name_size))
			return (i); /* Found! */
		i ++;
	}

	return ((size_t)~0);
}

int
ini_sect_val_enum(ini_p ini, size_t sect_off, size_t *val_off,
    const uint8_t **val_name, size_t *val_name_size,
    const uint8_t **val, size_t *val_size) {
	size_t i;

	if (NULL == ini || NULL == val_off)
		return (EINVAL);
	if (NULL == ini->lines)
		return (ENOENT);

	if ((sect_off + 1) > (*val_off)) {
		(*val_off) = (sect_off + 1);
	}
	/* Look for value. */
	for (i = (*val_off); i < ini->lines_count; i ++) {
		if (NULL == ini->lines[i])
			continue;
		if (INI_LINE_TYPE_SECTION == ini->lines[i]->type)
			return (ENOENT); /* No more values in section. */
		if (INI_LINE_TYPE_VALUE != ini->lines[i]->type)
			continue;
		/* Found! */
		(*val_off) = i;
		if (NULL != val_name) {
			(*val_name) = ini->lines[i]->name;
		}
		if (NULL != val_name_size) {
			(*val_name_size) = ini->lines[i]->name_size;
		}
		if (NULL != val) {
			(*val) = ini->lines[i]->val;
		}
		if (NULL != val_size) {
			(*val_size) = ini->lines[i]->val_size;
		}
		return (0);
	}

	return (ENOENT);
}

size_t
ini_sect_val_find(ini_p ini, size_t sect_off, const uint8_t *val_name,
    size_t val_name_size) {
	size_t i = 0, name_size;
	const uint8_t *name;

	if (NULL == ini || NULL == val_name || 0 == val_name_size ||
	    (size_t)~0 == sect_off)
		return ((size_t)~0);
	if (NULL == ini->lines)
		return ((size_t)~0);

	/* Look for value. */
	while (0 == ini_sect_val_enum(ini, sect_off, &i, &name, &name_size, NULL, NULL)) {
		if (0 == mem_cmpin(name, name_size, val_name, val_name_size))
			return (i); /* Found! */
		i ++;
	}

	return ((size_t)~0);
}

size_t
ini_sect_val_findi(ini_p ini, size_t sect_off, const uint8_t *val_name,
    size_t val_name_size) {
	size_t i = 0, name_size;
	const uint8_t *name;

	if (NULL == ini || NULL == val_name || 0 == val_name_size ||
	    (size_t)~0 == sect_off)
		return ((size_t)~0);
	if (NULL == ini->lines)
		return ((size_t)~0);

	/* Look for value. */
	while (0 == ini_sect_val_enum(ini, sect_off, &i, &name, &name_size, NULL, NULL)) {
		if (0 == mem_cmpin(name, name_size, val_name, val_name_size))
			return (i); /* Found! */
		i ++;
	}

	return ((size_t)~0);
}


int
ini_val_get(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    uint8_t **val, size_t *val_size) {
	size_t sect_off, val_off;

	if (NULL == ini || NULL == val || NULL == val_size)
		return (EINVAL);
	if (0 == sect_name_size && NULL != sect_name) {
		sect_name_size = strlen((const char*)sect_name);
	}
	if (0 == val_name_size && NULL != val_name) {
		val_name_size = strlen((const char*)val_name);
	}

	/* Look for section. */
	sect_off = ini_sect_find(ini, sect_name, sect_name_size);
	if ((size_t)~0 == sect_off)
		return (ENOENT); /* No section. */
	/* Look for value. */
	val_off = ini_sect_val_find(ini, sect_off, val_name, val_name_size);
	if ((size_t)~0 == val_off)
		return (ENOENT); /* No value in section. */
	(*val) = ini->lines[val_off]->val;
	(*val_size) = ini->lines[val_off]->val_size;

	return (0);
}

int
ini_val_get_int(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    ssize_t *val) {
	int error;
	uint8_t *tm_val;
	size_t val_size;

	if (NULL == val)
		return (EINVAL);
	error = ini_val_get(ini, sect_name, sect_name_size,
	    val_name, val_name_size, &tm_val, &val_size);
	if (0 != error)
		return (error);
	(*val) = ustr2ssize(tm_val, val_size);

	return (0);
}

int
ini_val_get_uint(ini_p ini, const uint8_t *sect_name, 
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    size_t *val) {
	int error;
	uint8_t *tm_val;
	size_t val_size;

	if (NULL == val)
		return (EINVAL);
	error = ini_val_get(ini, sect_name, sect_name_size,
	    val_name, val_name_size, &tm_val, &val_size);
	if (0 != error)
		return (error);
	(*val) = ustr2usize(tm_val, val_size);

	return (0);
}


int
ini_vali_get(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    uint8_t **val, size_t *val_size) {
	size_t sect_off, val_off;

	if (NULL == ini || NULL == val || NULL == val_size)
		return (EINVAL);
	if (0 == sect_name_size && NULL != sect_name) {
		sect_name_size = strlen((const char*)sect_name);
	}
	if (0 == val_name_size && NULL != val_name) {
		val_name_size = strlen((const char*)val_name);
	}

	/* Look for section. */
	sect_off = ini_sect_findi(ini, sect_name, sect_name_size);
	if ((size_t)~0 == sect_off)
		return (ENOENT); /* No section. */
	/* Look for value. */
	val_off = ini_sect_val_findi(ini, sect_off, val_name, val_name_size);
	if ((size_t)~0 == val_off)
		return (ENOENT); /* No value in section. */
	(*val) = ini->lines[val_off]->val;
	(*val_size) = ini->lines[val_off]->val_size;

	return (0);
}

int
ini_vali_get_int(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    ssize_t *val) {
	int error;
	uint8_t *tm_val;
	size_t val_size;

	if (NULL == val)
		return (EINVAL);
	error = ini_vali_get(ini, sect_name, sect_name_size,
	    val_name, val_name_size, &tm_val, &val_size);
	if (0 != error)
		return (error);
	(*val) = ustr2ssize(tm_val, val_size);

	return (0);
}

int
ini_vali_get_uint(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    size_t *val) {
	int error;
	uint8_t *tm_val;
	size_t val_size;

	if (NULL == val)
		return (EINVAL);
	error = ini_vali_get(ini, sect_name, sect_name_size,
	    val_name, val_name_size, &tm_val, &val_size);
	if (0 != error)
		return (error);
	(*val) = ustr2usize(tm_val, val_size);

	return (0);
}


int
ini_val_set(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    const uint8_t *val, size_t val_size) {
	size_t sect_off, val_off, data_size;
	ini_line_p line = NULL, *lines_new;

	if (NULL == ini || (NULL == val && 0 != val_size))
		return (EINVAL);
	if (0 == sect_name_size && NULL != sect_name) {
		sect_name_size = strlen((const char*)sect_name);
	}
	if (0 == val_name_size && NULL != val_name) {
		val_name_size = strlen((const char*)val_name);
	}

	/* Look for section. */
	sect_off = ini_sect_find(ini, sect_name, sect_name_size);
	if ((size_t)~0 == sect_off) { /* No section, add. */
		/* Alloc line for section. */
		line = ini_line_alloc__int((sect_name_size + 2));
		if (NULL == line)
			return (errno);
		line->type = INI_LINE_TYPE_SECTION;
		line->name = (line->data + 1);
		line->name_size = sect_name_size;
		line->data[0] = '[';
		memcpy(line->name, sect_name, sect_name_size);
		line->name[line->name_size] = ']';

		/* Add line to array. */
		lines_new = reallocarray(ini->lines,
		    (ini->lines_count + 1), sizeof(ini_line_p));
		if (NULL == lines_new) {
			free(line);
			return (errno);
		}
		ini->lines = lines_new;
		ini->lines[ini->lines_count] = line;
		sect_off = ini->lines_count;
		ini->lines_count ++;
	}

	data_size = (val_name_size + 1 + val_size);

	/* Look for value. */
	val_off = ini_sect_val_find(ini, sect_off, val_name, val_name_size);
	if ((size_t)~0 == val_off) { /* No value in section, add. */
		/* Add line to array. */
		lines_new = reallocarray(ini->lines,
		    (ini->lines_count + 1), sizeof(ini_line_p));
		if (NULL == lines_new) {
			free(line);
			return (errno);
		}
		ini->lines = lines_new;
		/* Add to section end. */
		for (val_off = (sect_off + 1); val_off < ini->lines_count; val_off ++) {
			if (NULL == ini->lines[val_off])
				continue;
			if (INI_LINE_TYPE_SECTION == ini->lines[val_off]->type)
				break;
		}
		/* Before empty lines.*/
		for (; 0 < val_off && val_off <= ini->lines_count; val_off --) {
			if (NULL == ini->lines[(val_off - 1)])
				continue;
			if (INI_LINE_TYPE_EMPTY_LINE != ini->lines[(val_off - 1)]->type)
				break;
		}
		memmove(&ini->lines[(val_off + 1)],
		    &ini->lines[val_off],
		    (sizeof(ini_line_p) * (ini->lines_count - val_off)));
		ini->lines[val_off] = NULL;
		ini->lines_count ++;

alloc_new_line:
		/* Alloc. */
		line = ini_line_alloc__int(data_size);
		if (NULL == line)
			return (errno);
		ini->lines[val_off] = line;
		line->type = INI_LINE_TYPE_VALUE;
		line->name = line->data;
		line->name_size = val_name_size;
		memcpy(line->name, val_name, val_name_size);
		line->name[line->name_size] = '=';
		line->val = (line->name + line->name_size + 1);
	} else { /* Check existing item free size. */
		line = ini->lines[val_off];
		if (NULL == line)
			goto alloc_new_line;
		if (line->data_allocated_size > data_size)
			goto update_value;
		/* Realloc. */
		line = realloc(ini->lines[val_off], (sizeof(ini_line_t) +
		    data_size + INI_CFG_LINE_ALLOC_PADDING));
		if (NULL == line)
			return (errno);
		if (ini->lines[val_off] == line)
			goto update_value;
		/* Update pointers. */
		ini->lines[val_off] = line;
		line->data = (uint8_t*)(line + 1);
		line->data_allocated_size = (data_size + INI_CFG_LINE_ALLOC_PADDING);
		line->name = line->data;
		line->val = (line->name + line->name_size + 1);
	}

update_value:
	/* Update. */
	line->data_size = data_size;
	line->val_size = val_size;
	memcpy(line->val, val, val_size);

	return (0);
}

int
ini_val_set_int(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    ssize_t val) {
	char buf[64];
	size_t buf_size;

	buf_size = (size_t)snprintf(buf, sizeof(buf), "%zi", val);

	return (ini_val_set(ini, sect_name, sect_name_size,
	    val_name, val_name_size, (uint8_t*)buf, buf_size));
}

int
ini_val_set_uint(ini_p ini, const uint8_t *sect_name,
    size_t sect_name_size, const uint8_t *val_name, size_t val_name_size,
    size_t val) {
	char buf[64];
	size_t buf_size;

	buf_size = (size_t)snprintf(buf, sizeof(buf), "%zu", val);

	return (ini_val_set(ini, sect_name, sect_name_size,
	    val_name, val_name_size, (uint8_t*)buf, buf_size));
}