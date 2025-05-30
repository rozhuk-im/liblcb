/*-
 * Copyright (c) 2012 - 2018 Rozhuk Ivan <rozhuk.im@gmail.com>
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


#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <sys/types.h>
#include <inttypes.h>


typedef struct iovec_s {
	uint8_t	*iov_base;  /* Base address. */
	size_t	iov_len;    /* Length. */
} iovec_t, *iovec_p;
//#if sizeof(struct iovec) != sizeof(iovec_t)
//#error("BAD struct iovec_s size")
//#endif


typedef struct r_buf_s { /* Ring buf. */
	uint8_t		*buf;		/* Ring buf. */
	size_t		size;		/* Ring buf size. */
	size_t		wpos;		/* Write offset. */
	uint8_t		*buf_max;	/* Ring buf max pos. */
	iovec_p		iov;		/* IOV array. */
	size_t		iov_count;	/* IOV array items count. */
	size_t		iov_index;	/* Current=next write IOV index. */
	size_t		iov_index_max;	/* Last valid IOV index. */
	size_t		round_num;	/* Ring buf round num. */
	size_t		min_block_size;
	size_t		iov_size;	/* IOV array size. */
	uint32_t	flags;		/* Flags. */
} r_buf_t, *r_buf_p;

/* Int set flags */
#define RBUF_F_FRAG		(((uint32_t)1) << 0) /* Fragmented. */
#define RBUF_F_FULL		(((uint32_t)1) << 1) /* Buffer is full: to detect round_num == 0 but data available. */
/* User set flags. */
#define RBUF_F_U__MASK__	0xffff0000
#define RBUF_F_U_KEEP_TAIL	(((uint32_t)1) << 16) /* Move tail data to head on buffer full. */


typedef struct r_buf_rpos_s { /* Ring buf read pos. */
	size_t		iov_index;	/* Send index for iov. */
	size_t		iov_off;	/* iov buffer offset. */
	size_t		round_num;	/* Ring buf round num. */
} r_buf_rpos_t, *r_buf_rpos_p;


/*  0: rpos1 == rpos2
 *  1: rpos1 > rpos2
 * -1: rpos1 < rpos2
 */
int	r_buf_rpos_cmp(r_buf_rpos_p rpos1, r_buf_rpos_p rpos2);
size_t	r_buf_rpos_calc_size(r_buf_p r_buf, r_buf_rpos_p rpos1, r_buf_rpos_p rpos2);
int	r_buf_rpos_init(r_buf_p r_buf, r_buf_rpos_p rpos,
	    size_t data_size);
int	r_buf_rpos_init_near(r_buf_p r_buf, r_buf_rpos_p rpos,
	    size_t data_size, r_buf_rpos_p rposs, size_t rposs_cnt);
int	r_buf_rpos_check_fast(r_buf_p r_buf, r_buf_rpos_p rpos);

r_buf_p	r_buf_alloc(uintptr_t fd, size_t size, const size_t min_block_size,
    const uint32_t flags);
void	r_buf_free(r_buf_p r_buf);

size_t	r_buf_wbuf_get(r_buf_p r_buf, size_t min_buf_size, uint8_t **buf);
int	r_buf_wbuf_set(r_buf_p r_buf, size_t offset, size_t buf_size);
int	r_buf_wbuf_set2(r_buf_p r_buf, uint8_t *buf, size_t buf_size,
	    r_buf_rpos_p rpos);
int	r_buf_wbuf_set_ex(r_buf_p r_buf, iovec_p iov, size_t iov_cnt);

size_t	r_buf_data_avail_size(r_buf_p r_buf, r_buf_rpos_p rpos,
	    size_t *drop_size);
size_t	r_buf_data_get(r_buf_p r_buf, r_buf_rpos_p rpos, size_t data_size,
	    iovec_p iov, size_t iov_cnt,
	    size_t *drop_size, size_t *data_size_ret);
/* sendfile() needs offset. */
int	r_buf_data_get_conv2off(r_buf_p r_buf, iovec_p iov, size_t iov_cnt);
void	r_buf_rpos_inc(r_buf_p r_buf, r_buf_rpos_p rpos, size_t data_size);


#endif /* __RING_BUFFER_H__ */
