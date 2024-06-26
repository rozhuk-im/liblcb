/*-
 * Copyright (c) 2013-2024 Rozhuk Ivan <rozhuk.im@gmail.com>
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
 */

 
// see
// RFC 1321 - MD5 (code base!)
// RFC 4634, RFC6234 - SHA2
// RFC 2104 - HMAC

#ifndef __SHA2_H__INCLUDED__
#define __SHA2_H__INCLUDED__

#include <sys/param.h>
#include <sys/types.h>
#include <string.h> /* memcpy, memmove, memset, strerror... */
#include <inttypes.h>
#if defined(__SHA__) && defined(__SSSE3__) && defined(__SSE4_1__)
#	include <cpuid.h>
#	include <xmmintrin.h> /* SSE */
#	include <emmintrin.h> /* SSE2 */
#	include <pmmintrin.h> /* SSE3 */
#	include <tmmintrin.h> /* SSSE3 */
#	include <smmintrin.h> /* SSE4.1 */
#	include <nmmintrin.h> /* SSE4.2 */
#	include <immintrin.h> /* AVX */
#	include <shaintrin.h>
#	define SHA2_ENABLE_SIMD	1
#endif

#ifndef bswap64
#	define bswap64		__builtin_bswap64
#endif

#ifndef nitems /* SIZEOF() */
#	define nitems(__val)	(sizeof(__val) / sizeof(__val[0]))
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#	define SHA2_ALIGN(__n)	__declspec(align(__n)) /* DECLSPEC_ALIGN() */
#else /* GCC/clang */
#	define SHA2_ALIGN(__n)	__attribute__ ((aligned(__n)))
#endif

static void *(*volatile sha2_memset_volatile)(void*, int, size_t) = memset;
#define sha2_bzero(__mem, __size)	sha2_memset_volatile((__mem), 0x00, (__size))


/* HASH constants. */
#define SHA2_224_HASH_SIZE	((size_t)28)
#define SHA2_256_HASH_SIZE	((size_t)32)
#define SHA2_384_HASH_SIZE	((size_t)48)
#define SHA2_512_HASH_SIZE	((size_t)64)
#define SHA2_HASH_MAX_SIZE	SHA2_512_HASH_SIZE

#define SHA2_224_HASH_STR_SIZE	(SHA2_224_HASH_SIZE * 2)
#define SHA2_256_HASH_STR_SIZE	(SHA2_256_HASH_SIZE * 2)
#define SHA2_384_HASH_STR_SIZE	(SHA2_384_HASH_SIZE * 2)
#define SHA2_512_HASH_STR_SIZE	(SHA2_512_HASH_SIZE * 2)
#define SHA2_HASH_STR_MAX_SIZE	SHA2_512_HASH_STR_SIZE

#define SHA2_256_MSG_BLK_SIZE	((size_t)64) /* 256 bit */
#define SHA2_512_MSG_BLK_SIZE	((size_t)128) /* 512 bit */
#define SHA2_MSG_BLK_MAX_SIZE	SHA2_512_MSG_BLK_SIZE
#define SHA2_256_MSG_BLK_64CNT	(SHA2_256_MSG_BLK_SIZE / sizeof(uint64_t)) /* 8 */
#define SHA2_512_MSG_BLK_64CNT	(SHA2_512_MSG_BLK_SIZE / sizeof(uint64_t)) /* 16 */
#define SHA2_MSG_BLK_MAX_64CNT	(SHA2_MSG_BLK_MAX_SIZE / sizeof(uint64_t)) /* 16 */


/* Define the SHA shift, rotate left and rotate right macro */
#define SHA2_SHR32(__n, __w)	((__w) >> (__n))
#define SHA2_ROTR32(__n, __w)	(((__w) >> (__n)) | ((__w) << (32 - (__n))))
#define SHA2_SHR64(__n, __w)	(((uint64_t)(__w)) >> (__n))
#define SHA2_ROTR64(__n, __w)	((((uint64_t)(__w)) >> (__n)) | (((uint64_t)(__w)) << (64 - (__n))))

/* Define the SHA SIGMA and sigma macros */
#define SHA2_32_SIGMA0(__w)						\
	(SHA2_ROTR32(2, __w) ^ SHA2_ROTR32(13, __w) ^ SHA2_ROTR32(22, __w))
#define SHA2_32_SIGMA1(__w)						\
	(SHA2_ROTR32(6, __w) ^ SHA2_ROTR32(11, __w) ^ SHA2_ROTR32(25, __w))
#define SHA2_32_sigma0(__w)						\
	(SHA2_ROTR32(7, __w) ^ SHA2_ROTR32(18, __w) ^ SHA2_SHR32(3, __w))
#define SHA2_32_sigma1(__w)						\
	(SHA2_ROTR32(17, __w) ^ SHA2_ROTR32(19, __w) ^ SHA2_SHR32(10, __w))

#define SHA2_64_SIGMA0(__w)						\
	(SHA2_ROTR64(28, __w) ^ SHA2_ROTR64(34, __w) ^ SHA2_ROTR64(39, __w))
#define SHA2_64_SIGMA1(__w)						\
	(SHA2_ROTR64(14, __w) ^ SHA2_ROTR64(18, __w) ^ SHA2_ROTR64(41, __w))
#define SHA2_64_sigma0(__w)						\
	(SHA2_ROTR64(1, __w) ^ SHA2_ROTR64(8, __w) ^ SHA2_SHR64(7, __w))
#define SHA2_64_sigma1(__w)						\
	(SHA2_ROTR64(19, __w) ^ SHA2_ROTR64(61, __w) ^ SHA2_SHR64(6, __w))

#define SHA2_Ch(__x, __y, __z)	(((__x) & (__y)) | ((~(__x)) & (__z)))
#define SHA2_Maj(__x, __y, __z)	(((__x) & (__y)) | ((__x) & (__z)) | ((__y) & (__z)))
/* From RFC 4634. */
//#define SHA2_Ch(__x, __y, __z)	(((__x) & (__y)) ^ ((~(__x)) & (__z)))
//#define SHA2_Maj(__x, __y, __z)	(((__x) & (__y)) ^ ((__x) & (__z)) ^ ((__y) & (__z)))
/* The following definitions are equivalent and potentially faster. */
//#define SHA2_Ch(__x, __y, __z)	(((__x) & ((__y) ^ (__z))) ^ (__z))
//#define SHA2_Maj(__x, __y, __z)	(((__x) & ((__y) | (__z))) | ((__y) & (__z)))


/* Initial Hash Values: FIPS-180-2 Change Notice 1 */
static const uint32_t SHA2_224_H0[(SHA2_256_HASH_SIZE / sizeof(uint32_t))] = {
	0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
	0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
};
/* Initial Hash Values: FIPS-180-2 section 5.3.2 */
static const uint32_t SHA2_256_H0[(SHA2_256_HASH_SIZE / sizeof(uint32_t))] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};
/* Initial Hash Values: FIPS-180-2 sections 5.3.3 and 5.3.4 */
static const uint64_t SHA2_384_H0[(SHA2_512_HASH_SIZE / sizeof(uint64_t))] = {
	0xcbbb9d5dc1059ed8ull, 0x629a292a367cd507ull, 0x9159015a3070dd17ull,
	0x152fecd8f70e5939ull, 0x67332667ffc00b31ull, 0x8eb44a8768581511ull,
	0xdb0c2e0d64f98fa7ull, 0x47b5481dbefa4fa4ull
};
static const uint64_t SHA2_512_H0[(SHA2_512_HASH_SIZE / sizeof(uint64_t))] = {
	0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull, 0x3c6ef372fe94f82bull,
	0xa54ff53a5f1d36f1ull, 0x510e527fade682d1ull, 0x9b05688c2b3e6c1full,
	0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull
};


/* This structure will hold context information for the SHA-1 hashing operation. */
typedef struct sha2_ctx_s {
	SHA2_ALIGN(32) uint64_t hash[(SHA2_HASH_MAX_SIZE / sizeof(uint64_t))]; /* Message Digest. */
	SHA2_ALIGN(32) uint64_t buffer[SHA2_MSG_BLK_MAX_64CNT]; /* Input buffer: message blocks. */
	SHA2_ALIGN(32) uint64_t W[80]; /* Temp buf for sha2_transform(). */
	uint64_t count;		/* Number of bits, modulo 2^64 (lsb first). */
	uint64_t count_hi;	/* Number of bits high. */
	size_t hash_size;	/* hash size of SHA being used */
	size_t block_size;	/* block size of SHA being used */
#ifdef SHA2_ENABLE_SIMD
	int use_simd;		/* Use SIMD SHA */
#endif
} sha2_ctx_t, *sha2_ctx_p;

typedef struct hmac_sha2_ctx_s {
	sha2_ctx_t ctx;
	SHA2_ALIGN(32) uint64_t k_opad[SHA2_MSG_BLK_MAX_64CNT]; /* outer padding - key XORd with opad. */
} hmac_sha2_ctx_t, *hmac_sha2_ctx_p;



static inline void
sha2_memcpy_bswap4(uint8_t *dst, const uint8_t *src, const size_t size) {
	register size_t i;

#pragma unroll
	//for (i = 0; i < size; i ++)
	//	dst[i] = src[((i & ~3) + (3 - (i & 0x03)))]; /* bswap()/htonl() */
	for (i = 0; i < size; i += 4) {
		dst[(i + 0)] = src[(i + 3)];
		dst[(i + 1)] = src[(i + 2)];
		dst[(i + 2)] = src[(i + 1)];
		dst[(i + 3)] = src[(i + 0)];
	}
}

static inline void
sha2_memcpy_bswap8(uint8_t *dst, const uint8_t *src, const size_t size) {
	register size_t i;

#pragma unroll
	//for (i = 0; i < size; i ++)
	//	dst[i] = src[((i & ~7) + (7 - (i & 0x07)))]; /* bswap()/htonl() */
	for (i = 0; i < size; i += 8) {
		dst[(i + 0)] = src[(i + 7)];
		dst[(i + 1)] = src[(i + 6)];
		dst[(i + 2)] = src[(i + 5)];
		dst[(i + 3)] = src[(i + 4)];
		dst[(i + 4)] = src[(i + 3)];
		dst[(i + 5)] = src[(i + 2)];
		dst[(i + 6)] = src[(i + 1)];
		dst[(i + 7)] = src[(i + 0)];
	}
}

/*
 *  sha2_init
 *
 *  Description:
 *      This function will initialize the sha2_ctx in preparation
 *      for computing a new SHA2 message digest.
 */
static inline void
sha2_init(const size_t bits, sha2_ctx_p ctx) {
	
	/* Load magic initialization constants. */
	switch (bits) {
	case 224:
	case SHA2_224_HASH_SIZE:
		ctx->hash_size = SHA2_224_HASH_SIZE;
		ctx->block_size = SHA2_256_MSG_BLK_SIZE;
		memcpy(&ctx->hash, SHA2_224_H0, sizeof(SHA2_224_H0));
		break;
	case 256:
	case SHA2_256_HASH_SIZE:
		ctx->hash_size = SHA2_256_HASH_SIZE;
		ctx->block_size = SHA2_256_MSG_BLK_SIZE;
		memcpy(&ctx->hash, SHA2_256_H0, sizeof(SHA2_256_H0));
		break;
	case 384:
	case SHA2_384_HASH_SIZE:
		ctx->hash_size = SHA2_384_HASH_SIZE;
		ctx->block_size = SHA2_512_MSG_BLK_SIZE;
		memcpy(&ctx->hash, SHA2_384_H0, sizeof(SHA2_384_H0));
		break;
	case 512:
	case SHA2_512_HASH_SIZE:
		ctx->hash_size = SHA2_512_HASH_SIZE;
		ctx->block_size = SHA2_512_MSG_BLK_SIZE;
		memcpy(&ctx->hash, SHA2_512_H0, sizeof(SHA2_512_H0));
		break;
	}
	ctx->count = 0;
	ctx->count_hi = 0;
#ifdef SHA2_ENABLE_SIMD
	uint32_t eax, ebx, ecx, edx;

	__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx);
	ctx->use_simd = (ebx & (((uint32_t)1) << 29));
#endif
}

/*
 * SHA2_224_256ProcessMessageBlock
 *
 * Description:
 *   This function will process the next 512 bits of the message
 *   stored in the Message_Block array.
 */
static inline void
sha2_transform_block64_generic(sha2_ctx_p ctx, const uint8_t *blocks,
    const uint8_t *blocks_max) {
	static const uint32_t K[64] = { /* Constants defined in FIPS-180-2, section 4.2.2 */
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
		0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
		0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
		0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
		0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
		0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
		0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
		0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
		0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};
	register uint32_t t; /* Loop counter. */
	register uint32_t temp1, temp2; /* Temporary word value. */
	register uint32_t A, B, C, D, E, F, G, H; /* Word buffers. */
	uint32_t *W, *hash; /* Word sequence. */

	W = (uint32_t*)ctx->W;
	hash = (uint32_t*)ctx->hash;
	A = hash[0];
	B = hash[1];
	C = hash[2];
	D = hash[3];
	E = hash[4];
	F = hash[5];
	G = hash[6];
	H = hash[7];

	for (; blocks < blocks_max; blocks += SHA2_256_MSG_BLK_SIZE) {
		/* Initialize the first 16 words in the array W. */
		sha2_memcpy_bswap4((uint8_t*)W, blocks, SHA2_256_MSG_BLK_SIZE);

#pragma unroll
		for (t = 16; t < 64; t ++) {
			W[t] = (SHA2_32_sigma1(W[(t - 2)]) + W[(t - 7)] +
				SHA2_32_sigma0(W[(t - 15)]) + W[(t - 16)]);
		}

#pragma unroll
		for (t = 0; t < 64; t ++) {
			temp1 = H + SHA2_32_SIGMA1(E) + SHA2_Ch(E, F, G) + K[t] + W[t];
			temp2 = SHA2_32_SIGMA0(A) + SHA2_Maj(A, B, C);
			H = G;
			G = F;
			F = E;
			E = D + temp1;
			D = C;
			C = B;
			B = A;
			A = temp1 + temp2;
		}

		A = (hash[0] += A);
		B = (hash[1] += B);
		C = (hash[2] += C);
		D = (hash[3] += D);
		E = (hash[4] += E);
		F = (hash[5] += F);
		G = (hash[6] += G);
		H = (hash[7] += H);
	}
}


#ifdef SHA2_ENABLE_SIMD

#define SHA2_SSE_LOADU(__ptr, __xmm0, __xmm1, __xmm2, __xmm3) do { 	\
	__xmm0 = _mm_loadu_si128(&((const __m128i*)(const void*)(__ptr))[0]); \
	__xmm1 = _mm_loadu_si128(&((const __m128i*)(const void*)(__ptr))[1]); \
	__xmm2 = _mm_loadu_si128(&((const __m128i*)(const void*)(__ptr))[2]); \
	__xmm3 = _mm_loadu_si128(&((const __m128i*)(const void*)(__ptr))[3]); \
} while (0)
#ifdef __SSE4_1__ /* SSE4.1 required. */
#define SHA2_SSE_STREAM_LOAD(__ptr, __xmm0, __xmm1, __xmm2, __xmm3) do { \
	__xmm0 = _mm_stream_load_si128(&((const __m128i*)(const void*)(__ptr))[0]); \
	__xmm1 = _mm_stream_load_si128(&((const __m128i*)(const void*)(__ptr))[1]); \
	__xmm2 = _mm_stream_load_si128(&((const __m128i*)(const void*)(__ptr))[2]); \
	__xmm3 = _mm_stream_load_si128(&((const __m128i*)(const void*)(__ptr))[3]); \
} while (0)
#endif

static inline void
sha2_transform_block64_simd(sha2_ctx_p ctx, const uint8_t *blocks,
    const uint8_t *blocks_max) {
	const __m128i MASK = _mm_set_epi64x(0x0c0d0e0f08090a0bull, 0x0405060700010203ull);
	__m128i TMP, TMSG0, TMSG1, TMSG2, TMSG3;
	__m128i STATE0, STATE1, ABEF_SAVE, CDGH_SAVE;

	/* Load initial values. */
	TMP = _mm_loadu_si128((const __m128i*)(const void*)&ctx->hash[0]);
	STATE1 = _mm_loadu_si128((const __m128i*)(const void*)&ctx->hash[2]);
	TMP = _mm_shuffle_epi32(TMP, 0xb1); /* CDAB */
	STATE1 = _mm_shuffle_epi32(STATE1, 0x1b); /* EFGH */
	STATE0 = _mm_alignr_epi8(TMP, STATE1, 8); /* ABEF */
	STATE1 = _mm_blend_epi16(STATE1, TMP, 0xf0); /* CDGH */

	for (; blocks < blocks_max; blocks += SHA2_256_MSG_BLK_SIZE) {
#ifdef __SSE4_1__ /* SSE4.1 required. */
		if (0 == (((size_t)blocks) & 15)) { /* 16 byte alligned. */
			SHA2_SSE_STREAM_LOAD(blocks, TMSG0, TMSG1, TMSG2, TMSG3);
		} else 
#endif
		{ /* Unaligned. */
			SHA2_SSE_LOADU(blocks, TMSG0, TMSG1, TMSG2, TMSG3);
			/* Shedule to load into cache. */
			if ((blocks + (SHA2_256_MSG_BLK_SIZE * 8)) < blocks_max) {
				_mm_prefetch((const char*)(blocks + (SHA2_256_MSG_BLK_SIZE * 8)), _MM_HINT_T0);
			}
		}

		/* Save current hash. */
		ABEF_SAVE = STATE0;
		CDGH_SAVE = STATE1;

		TMSG0 = _mm_shuffle_epi8(TMSG0, MASK);
		TMSG1 = _mm_shuffle_epi8(TMSG1, MASK);
		TMSG2 = _mm_shuffle_epi8(TMSG2, MASK);
		TMSG3 = _mm_shuffle_epi8(TMSG3, MASK);

		/* Rounds 0-3 */
		TMP = _mm_add_epi32(TMSG0,
		    _mm_set_epi64x((int64_t)0xe9b5dba5b5c0fbcfull, (int64_t)0x71374491428a2f98ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 4-7 */
		TMSG0 = _mm_sha256msg1_epu32(TMSG0, TMSG1);
		TMP = _mm_add_epi32(TMSG1,
		    _mm_set_epi64x((int64_t)0xab1c5ed5923f82a4ull, (int64_t)0x59f111f13956c25bull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 8-11 */
		TMSG1 = _mm_sha256msg1_epu32(TMSG1, TMSG2);
		TMP = _mm_add_epi32(TMSG2,
		    _mm_set_epi64x((int64_t)0x550c7dc3243185beull, (int64_t)0x12835b01d807aa98ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 12-15 */
		TMSG0 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG0, _mm_alignr_epi8(TMSG3, TMSG2, 4)), TMSG3);
		TMSG2 = _mm_sha256msg1_epu32(TMSG2, TMSG3);
		TMP = _mm_add_epi32(TMSG3,
		    _mm_set_epi64x((int64_t)0xc19bf1749bdc06a7ull, (int64_t)0x80deb1fe72be5d74ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 16-19 */
		TMSG1 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG1, _mm_alignr_epi8(TMSG0, TMSG3, 4)), TMSG0);
		TMSG3 = _mm_sha256msg1_epu32(TMSG3, TMSG0);
		TMP = _mm_add_epi32(TMSG0,
		    _mm_set_epi64x((int64_t)0x240ca1cc0fc19dc6ull, (int64_t)0xefbe4786e49b69c1ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 20-23 */
		TMSG2 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG2, _mm_alignr_epi8(TMSG1, TMSG0, 4)), TMSG1);
		TMSG0 = _mm_sha256msg1_epu32(TMSG0, TMSG1);
		TMP = _mm_add_epi32(TMSG1,
		    _mm_set_epi64x((int64_t)0x76f988da5cb0a9dcull, (int64_t)0x4a7484aa2de92c6full));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 24-27 */
		TMSG3 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG3, _mm_alignr_epi8(TMSG2, TMSG1, 4)), TMSG2);
		TMSG1 = _mm_sha256msg1_epu32(TMSG1, TMSG2);
		TMP = _mm_add_epi32(TMSG2,
		    _mm_set_epi64x((int64_t)0xbf597fc7b00327c8ull, (int64_t)0xa831c66d983e5152ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 28-31 */
		TMSG0 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG0, _mm_alignr_epi8(TMSG3, TMSG2, 4)), TMSG3);
		TMSG2 = _mm_sha256msg1_epu32(TMSG2, TMSG3);
		TMP = _mm_add_epi32(TMSG3,
		    _mm_set_epi64x((int64_t)0x1429296706ca6351ull, (int64_t)0xd5a79147c6e00bf3ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 32-35 */
		TMSG1 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG1, _mm_alignr_epi8(TMSG0, TMSG3, 4)), TMSG0);
		TMSG3 = _mm_sha256msg1_epu32(TMSG3, TMSG0);
		TMP = _mm_add_epi32(TMSG0,
		    _mm_set_epi64x((int64_t)0x53380d134d2c6dfcull, (int64_t)0x2e1b213827b70a85ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 36-39 */
		TMSG2 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG2, _mm_alignr_epi8(TMSG1, TMSG0, 4)), TMSG1);
		TMSG0 = _mm_sha256msg1_epu32(TMSG0, TMSG1);
		TMP = _mm_add_epi32(TMSG1,
		    _mm_set_epi64x((int64_t)0x92722c8581c2c92eull, (int64_t)0x766a0abb650a7354ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 40-43 */
		TMSG3 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG3, _mm_alignr_epi8(TMSG2, TMSG1, 4)), TMSG2);
		TMSG1 = _mm_sha256msg1_epu32(TMSG1, TMSG2);
		TMP = _mm_add_epi32(TMSG2,
		    _mm_set_epi64x((int64_t)0xc76c51a3c24b8b70ull, (int64_t)0xa81a664ba2bfe8a1ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 44-47 */
		TMSG0 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG0, _mm_alignr_epi8(TMSG3, TMSG2, 4)), TMSG3);
		TMSG2 = _mm_sha256msg1_epu32(TMSG2, TMSG3);
		TMP = _mm_add_epi32(TMSG3,
		    _mm_set_epi64x((int64_t)0x106aa070f40e3585ull, (int64_t)0xd6990624d192e819ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 48-51 */
		TMSG1 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG1, _mm_alignr_epi8(TMSG0, TMSG3, 4)), TMSG0);
		TMSG3 = _mm_sha256msg1_epu32(TMSG3, TMSG0);
		TMP = _mm_add_epi32(TMSG0,
		    _mm_set_epi64x((int64_t)0x34b0bcb52748774cull, (int64_t)0x1e376c0819a4c116ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 52-55 */
		TMSG2 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG2, _mm_alignr_epi8(TMSG1, TMSG0, 4)), TMSG1);
		TMP = _mm_add_epi32(TMSG1,
		    _mm_set_epi64x((int64_t)0x682e6ff35b9cca4full, (int64_t)0x4ed8aa4a391c0cb3ull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 56-59 */
		TMSG3 = _mm_sha256msg2_epu32(_mm_add_epi32(TMSG3, _mm_alignr_epi8(TMSG2, TMSG1, 4)), TMSG2);
		TMP = _mm_add_epi32(TMSG2,
		    _mm_set_epi64x((int64_t)0x8cc7020884c87814ull, (int64_t)0x78a5636f748f82eeull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Rounds 60-63 */
		TMP = _mm_add_epi32(TMSG3,
		    _mm_set_epi64x((int64_t)0xc67178f2bef9a3f7ull, (int64_t)0xa4506ceb90befffaull));
		STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, TMP);
		STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, _mm_shuffle_epi32(TMP, 0x0e));

		/* Add values back to state */
		STATE0 = _mm_add_epi32(STATE0, ABEF_SAVE);
		STATE1 = _mm_add_epi32(STATE1, CDGH_SAVE);
	}

	TMP = _mm_shuffle_epi32(STATE0, 0x1b); /* FEBA */
	STATE1 = _mm_shuffle_epi32(STATE1, 0xb1); /* DCHG */
	STATE0 = _mm_blend_epi16(TMP, STATE1, 0xf0); /* DCBA */
	STATE1 = _mm_alignr_epi8(STATE1, TMP, 8); /* ABEF */

	/* Save state */
	_mm_storeu_si128((__m128i*)(void*)&ctx->hash[0], STATE0);
	_mm_storeu_si128((__m128i*)(void*)&ctx->hash[2], STATE1);

	/* Restore the Floating-point status on the CPU. */
	_mm_empty();
}
#endif

/*
 * SHA2_384_512ProcessMessageBlock
 *
 * Description:
 *   This helper function will process the next 1024 bits of the
 *   message stored in the Message_Block array.
 */
static inline void
sha2_transform_block128_generic(sha2_ctx_p ctx, const uint8_t *blocks,
    const uint8_t *blocks_max) {
	/* Constants defined in FIPS-180-2, section 4.2.3 */
	static const uint64_t K[80] = {
		0x428a2f98d728ae22ull, 0x7137449123ef65cdull, 0xb5c0fbcfec4d3b2full,
		0xe9b5dba58189dbbcull, 0x3956c25bf348b538ull, 0x59f111f1b605d019ull,
		0x923f82a4af194f9bull, 0xab1c5ed5da6d8118ull, 0xd807aa98a3030242ull,
		0x12835b0145706fbeull, 0x243185be4ee4b28cull, 0x550c7dc3d5ffb4e2ull,
		0x72be5d74f27b896full, 0x80deb1fe3b1696b1ull, 0x9bdc06a725c71235ull,
		0xc19bf174cf692694ull, 0xe49b69c19ef14ad2ull, 0xefbe4786384f25e3ull,
		0x0fc19dc68b8cd5b5ull, 0x240ca1cc77ac9c65ull, 0x2de92c6f592b0275ull,
		0x4a7484aa6ea6e483ull, 0x5cb0a9dcbd41fbd4ull, 0x76f988da831153b5ull,
		0x983e5152ee66dfabull, 0xa831c66d2db43210ull, 0xb00327c898fb213full,
		0xbf597fc7beef0ee4ull, 0xc6e00bf33da88fc2ull, 0xd5a79147930aa725ull,
		0x06ca6351e003826full, 0x142929670a0e6e70ull, 0x27b70a8546d22ffcull,
		0x2e1b21385c26c926ull, 0x4d2c6dfc5ac42aedull, 0x53380d139d95b3dfull,
		0x650a73548baf63deull, 0x766a0abb3c77b2a8ull, 0x81c2c92e47edaee6ull,
		0x92722c851482353bull, 0xa2bfe8a14cf10364ull, 0xa81a664bbc423001ull,
		0xc24b8b70d0f89791ull, 0xc76c51a30654be30ull, 0xd192e819d6ef5218ull,
		0xd69906245565a910ull, 0xf40e35855771202aull, 0x106aa07032bbd1b8ull,
		0x19a4c116b8d2d0c8ull, 0x1e376c085141ab53ull, 0x2748774cdf8eeb99ull,
		0x34b0bcb5e19b48a8ull, 0x391c0cb3c5c95a63ull, 0x4ed8aa4ae3418acbull,
		0x5b9cca4f7763e373ull, 0x682e6ff3d6b2b8a3ull, 0x748f82ee5defb2fcull,
		0x78a5636f43172f60ull, 0x84c87814a1f0ab72ull, 0x8cc702081a6439ecull,
		0x90befffa23631e28ull, 0xa4506cebde82bde9ull, 0xbef9a3f7b2c67915ull,
		0xc67178f2e372532bull, 0xca273eceea26619cull, 0xd186b8c721c0c207ull,
		0xeada7dd6cde0eb1eull, 0xf57d4f7fee6ed178ull, 0x06f067aa72176fbaull,
		0x0a637dc5a2c898a6ull, 0x113f9804bef90daeull, 0x1b710b35131c471bull,
		0x28db77f523047d84ull, 0x32caab7b40c72493ull, 0x3c9ebe0a15c9bebcull,
		0x431d67c49c100d4cull, 0x4cc5d4becb3e42b6ull, 0x597f299cfc657e2aull,
		0x5fcb6fab3ad6faecull, 0x6c44198c4a475817ull
	};
	register uint32_t t; /* Loop counter. */
	register uint64_t temp1, temp2; /* Temporary word value. */
	register uint64_t A, B, C, D, E, F, G, H; /* Word buffers. */
	uint64_t *W, *hash; /* Word sequence. */

	W = ctx->W;
	hash = ctx->hash;
	A = hash[0];
	B = hash[1];
	C = hash[2];
	D = hash[3];
	E = hash[4];
	F = hash[5];
	G = hash[6];
	H = hash[7];

	for (; blocks < blocks_max; blocks += SHA2_512_MSG_BLK_SIZE) {
		/* Initialize the first 16 words in the array W. */
		sha2_memcpy_bswap8((uint8_t*)W, blocks, SHA2_512_MSG_BLK_SIZE);

#pragma unroll
		for (t = 16; t < 80; t ++) {
			W[t] = (SHA2_64_sigma1(W[(t - 2)]) + W[(t - 7)] +
				SHA2_64_sigma0(W[(t - 15)]) + W[(t - 16)]);
		}

#pragma unroll
		for (t = 0; t < 80; t ++) {
			temp1 = H + SHA2_64_SIGMA1(E) + SHA2_Ch(E, F, G) + K[t] + W[t];
			temp2 = SHA2_64_SIGMA0(A) + SHA2_Maj(A, B, C);
			H = G;
			G = F;
			F = E;
			E = D + temp1;
			D = C;
			C = B;
			B = A;
			A = temp1 + temp2;
		}

		A = (hash[0] += A);
		B = (hash[1] += B);
		C = (hash[2] += C);
		D = (hash[3] += D);
		E = (hash[4] += E);
		F = (hash[5] += F);
		G = (hash[6] += G);
		H = (hash[7] += H);
	}
}

static inline void
sha2_transform(sha2_ctx_p ctx, const uint8_t *blocks, const uint8_t *blocks_max) {

	if (SHA2_512_MSG_BLK_SIZE == ctx->block_size) {
		sha2_transform_block128_generic(ctx, blocks, blocks_max);
		return;
	}
#ifdef SHA2_ENABLE_SIMD
	if (ctx->use_simd) {
		sha2_transform_block64_simd(ctx, blocks, blocks_max);
		return;
	}
#endif
	sha2_transform_block64_generic(ctx, blocks, blocks_max);
}

/*
 *  sha2_update
 *
 *  Description:
 *      This function accepts an array of octets as the next portion
 *      of the message.
 *
 *  Parameters:
 *      ctx: [in/out]
 *          The SHA ctx to update
 *      message_array: [in]
 *          An array of characters representing the next portion of
 *          the message.
 *      length: [in]
 *          The length of the message in message_array
 */
static inline void
sha2_update(sha2_ctx_p ctx, const uint8_t *data, size_t data_size) {
	size_t buffer_usage, part_size, mask;

	if (0 == data_size)
		return;
	/* Compute number of bytes mod ctx->block_size. */
	mask = (ctx->block_size - 1); /* Block size mask. */
	buffer_usage = (ctx->count & mask);
	part_size = (ctx->block_size - buffer_usage);
	/* Update number of bits. */
	ctx->count += data_size;
	if (ctx->count < data_size) {
		ctx->count_hi ++;
	}
	/* Transform as many times as possible. */
	if (part_size <= data_size) {
		if (0 != buffer_usage) { /* Add data to buffer and process it. */
			memcpy((((uint8_t*)ctx->buffer) + buffer_usage), data, part_size);
			buffer_usage = 0;
			data += part_size;
			data_size -= part_size;
			sha2_transform(ctx, (uint8_t*)ctx->buffer,
			    (((uint8_t*)ctx->buffer) + ctx->block_size));
		}
		if (ctx->block_size <= data_size) {
			sha2_transform(ctx, data,
			    (data + (data_size & ~mask)));
			data += (data_size & ~mask);
			data_size &= mask;
		}
	}
	/* Buffer remaining data. */
	memcpy((((uint8_t*)ctx->buffer) + buffer_usage), data, data_size);
}

/*
 *  sha2_final
 *
 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512/1024 bits.  The first padding bit must be a '1'.  The last 64/128
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the buffer array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *      This function will return the message digest into the
 *      digest array  provided by the caller.
 *      NOTE: The first octet of hash is stored in the 0th element, 
 *            the last octet of hash in the 19th element.
 *
 *  Parameters:
 *      ctx: [in/out]
 *          The ctx to use to calculate the SHA-1 hash.
 *      digest: [out]
 *          Where the digest is returned.
 */
static inline void
sha2_final(sha2_ctx_p ctx, uint8_t *digest) {
	size_t buffer_usage, mask, len_off;

	/* Compute number of bytes mod ctx->block_size. */
	mask = (ctx->block_size - 1); /* Block size mask. */
	len_off = ((SHA2_256_MSG_BLK_SIZE == ctx->block_size) ?
	    (SHA2_256_MSG_BLK_SIZE - 8) : (SHA2_512_MSG_BLK_SIZE - 16)); /* Message length offset: 64 = 56, 128 = 112. */
	buffer_usage = (ctx->count & mask);
	((uint8_t*)ctx->buffer)[buffer_usage ++] = 0x80; /* Padding... */
	if (len_off < buffer_usage) { /* Not enouth space for message length. */
		memset((((uint8_t*)ctx->buffer) + buffer_usage), 0x00,
		    (ctx->block_size - buffer_usage));
		sha2_transform(ctx, (uint8_t*)ctx->buffer,
		    (((uint8_t*)ctx->buffer) + ctx->block_size));
		buffer_usage = 0;
	}
	memset((((uint8_t*)ctx->buffer) + buffer_usage), 0x00, (len_off - buffer_usage));
	/* Store the message length as the last 8/16 octets. */
	if (SHA2_256_MSG_BLK_SIZE == ctx->block_size) {
		ctx->buffer[(SHA2_256_MSG_BLK_64CNT - 1)] = bswap64((ctx->count << 3));
	} else {
		ctx->buffer[(SHA2_512_MSG_BLK_64CNT - 2)] =
		    bswap64(((ctx->count_hi << 3) | (ctx->count >> (64 - 3))));
		ctx->buffer[(SHA2_512_MSG_BLK_64CNT - 1)] = bswap64((ctx->count << 3));
	}
	sha2_transform(ctx, (uint8_t*)ctx->buffer,
	    (((uint8_t*)ctx->buffer) + ctx->block_size));
	/* Store state in digest. */
	if (SHA2_256_MSG_BLK_SIZE == ctx->block_size) {
		sha2_memcpy_bswap4(digest, (uint8_t*)ctx->hash, ctx->hash_size);
	} else {
		sha2_memcpy_bswap8(digest, (uint8_t*)ctx->hash, ctx->hash_size);
	}
	/* Zeroize sensitive information. */
	sha2_bzero(ctx, sizeof(sha2_ctx_t));
}


/* RFC 2104 */
/*
 * the HMAC_SHA2 transform looks like:
 *
 * SHA2(K XOR opad, SHA2(K XOR ipad, data))
 *
 * where K is an n byte 'key'
 * ipad is the byte 0x36 repeated 64 times
 * opad is the byte 0x5c repeated 64 times
 * and 'data' is the data being protected
 */
/*
 * data - pointer to data stream
 * data_size - length of data stream
 * key - pointer to authentication key
 * key_len - length of authentication key
 * digest - caller digest to be filled in
 */
static inline void
hmac_sha2_init(const size_t bits, const uint8_t *key, const size_t key_len,
    hmac_sha2_ctx_p hctx) {
	register size_t i = key_len;
	uint64_t k_ipad[SHA2_MSG_BLK_MAX_64CNT]; /* inner padding - key XORd with ipad. */

	/* Start out by storing key in pads. */
	/* If key is longer than block_size bytes reset it to key = SHA2(key). */
	sha2_init(bits, &hctx->ctx); /* Init context for 1st pass / Get hash params. */
	if (hctx->ctx.block_size < i) {
		sha2_update(&hctx->ctx, key, i);
		i = hctx->ctx.hash_size;
		sha2_final(&hctx->ctx, (uint8_t*)k_ipad);
		sha2_init(bits, &hctx->ctx); /* Reinit context for 1st pass. */
	} else {
		memcpy(k_ipad, key, i);
	}
	memset((((uint8_t*)k_ipad) + i), 0x00, (SHA2_MSG_BLK_MAX_SIZE - i));
	memcpy(hctx->k_opad, k_ipad, sizeof(k_ipad));

	/* XOR key with ipad and opad values. */
#pragma unroll
	for (i = 0; i < SHA2_MSG_BLK_MAX_64CNT; i ++) {
		k_ipad[i] ^= 0x3636363636363636ull;
		hctx->k_opad[i] ^= 0x5c5c5c5c5c5c5c5cull;
	}
	/* Perform inner SHA2. */
	sha2_update(&hctx->ctx, (uint8_t*)k_ipad, hctx->ctx.block_size); /* Start with inner pad. */
	/* Zeroize sensitive information. */
	sha2_bzero(k_ipad, sizeof(k_ipad));
}

static inline void
hmac_sha2_update(hmac_sha2_ctx_p hctx, const uint8_t *data, const size_t data_size) {

	sha2_update(&hctx->ctx, data, data_size); /* Then data of datagram. */
}

static inline void
hmac_sha2_final(hmac_sha2_ctx_p hctx, uint8_t *digest, size_t *digest_size) {
	size_t bits;

	bits = hctx->ctx.hash_size;
	sha2_final(&hctx->ctx, digest); /* Finish up 1st pass. */
	/* Perform outer SHA2. */
	sha2_init(bits, &hctx->ctx); /* Init context for 2nd pass. */
	sha2_update(&hctx->ctx, (uint8_t*)hctx->k_opad, hctx->ctx.block_size); /* Start with outer pad. */
	sha2_update(&hctx->ctx, digest, hctx->ctx.hash_size); /* Then results of 1st hash. */
	if (NULL != digest_size) {
		(*digest_size) = hctx->ctx.hash_size;
	}
	sha2_final(&hctx->ctx, digest); /* Finish up 2nd pass. */
	/* Zeroize sensitive information. */
	sha2_bzero(hctx->k_opad, sizeof(hctx->k_opad));
}

static inline void
hmac_sha2(const size_t bits, const uint8_t *key, const size_t key_len,
    const uint8_t *data, const size_t data_size,
    uint8_t *digest, size_t *digest_size) {
	hmac_sha2_ctx_t hctx;

	hmac_sha2_init(bits, key, key_len, &hctx);
	hmac_sha2_update(&hctx, data, data_size);
	hmac_sha2_final(&hctx, digest, digest_size);
}


static inline void
sha2_cvt_hex(const uint8_t *bin, const size_t bin_size, uint8_t *hex) {
	static const uint8_t *hex_tbl = (const uint8_t*)"0123456789abcdef";
	register const uint8_t *bin_max;
	register uint8_t byte;

	for (bin_max = (bin + bin_size); bin < bin_max; bin ++) {
		byte = (*bin);
		(*hex ++) = hex_tbl[((byte >> 4) & 0x0f)];
		(*hex ++) = hex_tbl[(byte & 0x0f)];
	}
	(*hex) = 0;
}


/* Other staff. */
static inline void
sha2_cvt_str(const uint8_t *digest, const size_t digest_size, char *digest_str) {

	sha2_cvt_hex(digest, digest_size, (uint8_t*)digest_str);
}


static inline void
sha2_get_digest(const size_t bits, const void *data, const size_t data_size,
    uint8_t *digest, size_t *digest_size) {
	sha2_ctx_t ctx;

	sha2_init(bits, &ctx);
	sha2_update(&ctx, data, data_size);
	if (NULL != digest_size) {
		(*digest_size) = ctx.hash_size;
	}
	sha2_final(&ctx, digest);
}


static inline void
sha2_get_digest_str(const size_t bits, const char *data, const size_t data_size,
    char *digest_str, size_t *digest_str_size) {
	sha2_ctx_t ctx;
	size_t digest_size;
	uint8_t digest[SHA2_HASH_MAX_SIZE];

	sha2_init(bits, &ctx);
	sha2_update(&ctx, (const uint8_t*)data, data_size);
	digest_size = ctx.hash_size;
	sha2_final(&ctx, digest);

	sha2_cvt_str(digest, digest_size, digest_str);
	if (NULL != digest_str_size) {
		(*digest_str_size) = (digest_size * 2);
	}
}


static inline void
sha2_hmac_get_digest(const size_t bits, const void *key, const size_t key_size,
    const void *data, const size_t data_size, uint8_t *digest,
    size_t *digest_size) {

	hmac_sha2(bits, (const uint8_t*)key, key_size,
	    (const uint8_t*)data, data_size, digest, digest_size);
}


static inline void
sha2_hmac_get_digest_str(const size_t bits, const char *key, const size_t key_size,
    const char *data, const size_t data_size, 
    char *digest_str, size_t *digest_str_size) {
	size_t digest_size;
	uint8_t digest[SHA2_HASH_MAX_SIZE];

	hmac_sha2(bits, (const uint8_t*)key, key_size,
	    (const uint8_t*)data, data_size, digest, &digest_size);
	sha2_cvt_str(digest, digest_size, digest_str);
	if (NULL != digest_str_size) {
		(*digest_str_size) = (digest_size * 2);
	}
}


#ifdef SHA2_SELF_TEST
/* 0 - OK, non zero - error */
static inline int
sha2_self_test(void) {
	size_t i;
	char digest_str[SHA2_HASH_STR_MAX_SIZE + 1]; /* Calculated digest. */
	const char *data[] = {
	    "",
	    "a",
	    "abc",
	    "message digest",
	    "012345670123456701234567012345670123456701234567012345",
	    "0123456701234567012345670123456701234567012345670123456",
	    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	    "012345670123456701234567012345670123456701234567012345678",
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
	    "012345670123456701234567012345670123456701234567012345670123456",
	    "0123456701234567012345670123456701234567012345670123456701234567",
	    "01234567012345670123456701234567012345670123456701234567012345678",
	    "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
	    "012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456",
	    "0123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567",
	    "01234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345678",
	    "0123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456",
	    "01234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567",
	    "012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345678"
	};
	const size_t data_size[] = {
	    0, 1, 3, 14, 54, 55, 56, 57, 62, 63, 64, 65, 80, 111, 112, 113, 127, 128, 129
	};
	const char *result_digest224[] = {
	    "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f",
	    "abd37534c7d9a2efb9465de931cd7055ffdb8879563ae98078d6d6d5",
	    "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7",
	    "2cb21c83ae2f004de7e81c3c7019cbcb65b71ab656b22d6d0c39b8eb",
	    "f7e62d005fb5d13d92b1ca042ba62d1abc35686962f8888049f1c817",
	    "0f4aed339c1323553f150acab7dc13679566642f77def27fcbd55320",
	    "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525",
	    "65e7cb7d49f46bd2e4d63e6a37ca8e472eea589be61502fe8c01410a",
	    "bff72b4fcb7d75e5632900ac5f90d219e05e97a7bde72e740db393d9",
	    "27cbe74013dfb2ff46d246211bed82e6ecb93a1e6eb5def8c7ea146a",
	    "1152a558c1dbcd023222e3472f2b9a2bdf8984e5d82153e6b126a201",
	    "0e719da8a107590fc3ba844aa3f596d2edba231a89501f5837fd00bf",
	    "b50aecbe4e9bb0b57bc5f3ae760a8e01db24f203fb3cdcd13148046e",
	    "10ec5f7daa49ea9c78c2dcae936bb9e23e07b4f87aa389bac0a2190d",
	    "375c8530330c05c42a2ed24da851950c44e3865ee00f0a9e44f25b72",
	    "bc73c6b436e03d7e62803275ef475864ede3ab55eb18e63293c1a42b",
	    "04ad4c32acc05bd4bd0b73f681e36d55c0f958335d149f49f2a316a4",
	    "28fd2cc5f136e9a1077e596e68769fd11b0954d0bb3ef7f7074ad5b4",
	    "db738d64d4f973f35e0c24408d795a21dd65e3880689be18492f2f95"
	};
	const char *result_digest256[] = {
	    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	    "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb",
	    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	    "f7846f55cf23e14eebeab5b4e1550cad5b509e3348fbc4efa3a1413d393cb650",
	    "57a069f9dde6cd4e7142defb5579e0ed87587491f8fe0e89cae06486ba9ec58c",
	    "6c8b1d86d0460e5cf48cacad0b844dc52b91967791d15fc7a90bb1cc3bc20879",
	    "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
	    "929e74c8b5300a1fe90ef7a5a811d84b1d0832f5905d22d01169c5dc72d4885b",
	    "db4bfcbd4da0cd85a60c3c37d3fbd8805c77f15fc6b1fdfe614ee0a7c8fdb4c0",
	    "d7c32c78e3517862a0c2823da7927e648ed3f98df2b5acbb281a2ef14718e6f7",
	    "8182cadb21af0e37c06414ece08e19c65bdb22c396d48ba7341012eea9ffdfdd",
	    "3af49e9f35df79113cfb77d54a9e90cf486f23d3a28f972d9b948fb239f9b60c",
	    "f371bc4a311f2b009eef952dd83ca80e2b60026c8e935592d0f9c308453c813e",
	    "da968265fa7a27806a19c5bc14f62188fd9bed0fd4cedfdaf8ce99192847c6b7",
	    "0ea705b1a43a896343a017d8f3e18b4276b5a30a0aad847345e2798a3998d070",
	    "848efe0b37e4bd5e7e0b50fc38263255ad614e560a6b3cd6eedf9a0a0ff565d3",
	    "e469b91793c029ee897ad1b1e6f047f55a60ebbe59d62262cdb31fcf973d860c",
	    "8c47d8dbc626b56a6c2aa57818843ee0f78b35e06055972f226ab1a2d92272a8",
	    "6d5013a9b331929c8c8d81bb4257bcf281a2949b653437a354eacc1a5e98461e"
	};
	const char *result_digest384[] = {
	    "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b",
	    "54a59b9f22b0b80880d8427e548b7c23abd873486e1f035dce9cd697e85175033caa88e6d57bc35efae0b5afd3145f31",
	    "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7",
	    "473ed35167ec1f5d8e550368a3db39be54639f828868e9454c239fc8b52e3c61dbd0d8b4de1390c256dcbb5d5fd99cd5",
	    "a4e2e4b1253f340e6543926f994dd950790491acb2d29e1486c3dc7d3a4bb8101455675cfe72e38708255e69fe9c764a",
	    "37738fdf836f6c0385498f54dceb21fea733eeb087c29f624e49afe48302d0248add4ca62cdc6e1dd16c7272f62519be",
	    "3391fdddfc8dc7393707a65b1b4709397cf8b1d162af05abfe8f450de5f36bc6b0455a8520bc4e6f5fe95b1fe3c8452b",
	    "1a05fe50fa0174c49c04ea75cbedaa2daef990f3b78cea6d68e43238e484ad5117529e888f9b28eb6ef196e46fd3428b",
	    "1761336e3f7cbfe51deb137f026f89e01a448e3b1fafa64039c1464ee8732f11a5341a6f41e0c202294736ed64db1a84",
	    "2551494bf6469c378d6f6f1a529b6e5757a3cb0f8f502b3bd1dc4a5cbd7e08b85e2994aa4cd9c8df72c7d93a9ff48599",
	    "72f5893331c249312d3c2b7a9709a7b96908b7769179dd9824ed578669fcc1f1c2de02c03b3d35a467aa0b472c1bb3d1",
	    "48e0250f216da181584f433b9a27c6ad31cd5439a4560821b3853ac391ad9c33510e92b40c87146f2bcf0ac6de7a69b0",
	    "b12932b0627d1c060942f5447764155655bd4da0c9afa6dd9b9ef53129af1b8fb0195996d2de9ca0df9d821ffee67026",
	    "25421d423e6a90a6ba5c114fcb8428a66221f37b34f98e928889d2e6138bcc3492bee42ed2bdb93fa92d18a3b9dc688f",
	    "571bd16f36d0c2e7f38ffbc5732e06b2416e005851da3cb6f89767a6ec2b99f07ab58dc974fa0c4a40f2efb65b1617a5",
	    "3f294290ed8cf396a68e32da348458f82e112a871626263187e9ba4dd66d23b38ab9212f5cf82e8a6b2c79333b59304c",
	    "6bf75d6d0a72e7b7cf02e41ba4491c005b0fa22616e455b87789f72197942bf3cf303b1edd415596c1a3a776d58a1bf2",
	    "7f58ec41bf728283cc2bf76e075638693c14188d46e07520f600ce5b6a31ace48f581a2acc62a33abfc6250de088158c",
	    "d4fc001ad316f8aec7ebedff72c0650bc843c1c1d6a33c4ba6f81b33579196815ff0d4d190df6f9e1f6b027ddde5299d"
	};
	const char *result_digest512[] = {
	    "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
	    "1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f5302860c652bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75",
	    "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
	    "107dbf389d9e9f71a3a95f6c055b9251bc5268c2be16d6c13492ea45b0199f3309e16455ab1e96118e8a905d5597b72038ddb372a89826046de66687bb420e7c",
	    "a615a53d9d84966309a78ba71591f6664f69757464dbc957c1788ab80cba057b7dd52b699d8756573466e244385b2285f5bc56accf6a91f01a3612605821d308",
	    "387d57a6c08ae2a2004f7db1bb41a492fd22622c017ba3560b4d0472e11bcf34c0c022b9c2d5c7c01be952eb74af8485a794d6c6f40cee83c8f31388a9d45bd1",
	    "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445",
	    "7e0fe81c333a3865f6730794d4b653b4b23e60bbacdf8310ac6b65483733aaaa7c3ff3002886a999d17e655e5f3e2cf79e8b286141b24619c77d50f5e61686b8",
	    "1e07be23c26a86ea37ea810c8ec7809352515a970e9253c26f536cfc7a9996c45c8370583e0a78fa4a90041d71a4ceab7423f19c71b9d5a3e01249f0bebd5894",
	    "a833c259b5ed24eafc9f311f8b625334eda25f396e88dbbc18ab8a6650ca228cfde203b07bf5cd7b631a47df8af6fcb8e546be24edeb315e76be80ffc19dc63c",
	    "846e0ef73436438a4acb0ba7078cfe381f10a0f5edebcb985b3790086ef5e7ac5992ac9c23c77761c764bb3b1c25702d06b99955eb197d45b82fb3d124699d78",
	    "f8eb0734ab6183cbde1cdcaeb28bd85fb0321a6c6edb6557edf7d681352b924855a59fb93241de3d766fbbd3afcd2634c06a1b3ed688d0019d1945858c50ec62",
	    "72ec1ef1124a45b047e8b7c75a932195135bb61de24ec0d1914042246e0aec3a2354e093d76f3048b456764346900cb130d2a4fd5dd16abb5e30bcb850dee843",
	    "3a95588c52bb0625acc56ce70181e209c5acc80c527d54d163081275d3f16bb0d50410ee0b69937ee4137b2f57514a22c6bd57234b2ae98dfd552e5d313a0cef",
	    "d08cfae8e852207ad472553247f2ce14d5a2159c8b71cc7cfa3b5c79c219a5288625f07d47beee823d32b6d123b210f946e3e57be3404550435ed126f13b5f83",
	    "09fc36891cb83e78f840a55a737bccb7b3c5fa52db4259de054287ba866f2ca50a695a26c492230093d5d4dbe68931e708f088ae9a9a3920188583fa61211fa8",
	    "f18338812dccb36217675bb27f255d078765749c8df41609d4f5ef355e24dcebfc302c574b29732d52446da53706962551e8f6b81d43358944d6686e378c2d87",
	    "0bd3b2de6fabc60af35a8a29b926f40d963264d800be148e64a23efee737849ad6acec96140a2b1c5f11d2d49f1fc3fb2a2c885e4a850729272884ade9e0fe21",
	    "74b6814a0a5bc234c7f25507e18c23892f767feda954e12164dc62184eefbdbc281f2ae34b31fb0dad116bd56c113b7bfc4ad88b7bfafcead05e7deac3f3c446"
	};

	const char *result_hdigest256[] = {
	    "b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad",
	    "3ecf5388e220da9e0f919485deb676d8bee3aec046a779353b463418511ee622",
	    "2f02e24ae2e1fe880399f27600afa88364e6062bf9bbe114b32fa8f23d03608a",
	    "75771da168a235a3a9e18fa7a536ea8c2511b4b16a7b3e2ee5253ef193f522bd",
	    "3f077f10f51c27cbc9c5b5bb987263148f5a1ac1d53f264a187f572ebb53bc5f",
	    "60c5cfc00f7aba24144e654565433ccf79d6a0f80a99201f746129a82eeee538",
	    "29dc3b24e96ab703b3cdad77288ad2d7e4c9129ab46558afe24e23431e436108",
	    "b806e3c6174bdcbe7ea8d9d3c0caac6c24f77035b52b6e6b3b5974da32712060",
	    "75c7ba6a508100cba27cc531f361e7ea84afbd4ec4eaf9f3d5e25e1b3ea576a0",
	    "0e02b710e17e2d316bf9c2bcc2493f221b7ca05da12e133f4023c611a21039bd",
	    "de34d55231f0d6508bd99ea30c6c555db399d36ab7ab389c9d521b01326e8554",
	    "46edb61899b59ffc44df1079ff0a4cc52c7119fb4b514ee4724dab90d2c8c0a3",
	    "2ce14d787b67f396724f2e046073d2ce32e45bc7d66bb7ef955341beaeeebae3",
	    "5222dd9cf5b20fd167d1fd93d5a7e23bb12fcd530da76edbd499d72fd2cae075",
	    "6091eebdff8034cf6b00104b63bd51c6be5fb8eb4d800f628b8a2d81928e22ac",
	    "a796d1087eaaeb1973a0f67029aca75ea7a85612d21e51ef5547358d53166189",
	    "ee6f61d125624eb98e6aa60b8b20f302541d85c88e880d345e558b78b20e7424",
	    "5976ba6aacf49a9961f4d550da64132881123c087aaf32444700d7cc86fe5e74",
	    "bd40323e6e8eab69350cd1d6b45d95d14a2dd120e9245a9231daa54589b12f57"
	};
	const char *result_hdigest384[] = {
	    "6c1f2ee938fad2e24bd91298474382ca218c75db3d83e114b3d4367776d14d3551289e75e8209cd4b792302840234adc",
	    "724c212553f366248bc76017e812c8acc85b94fec2f396c2a925bcc2571f7ab29fedee6b3b3013bbf9de7b89549d5a69",
	    "ef38b92496d409ab82a4ebc44724b39c0be52da9805db4605958935dd74f8a16258f403b5572cce139825caec1657893",
	    "68adcc18911636ff43bff1d137eb5aebe5989fde70745b7c603786b15251c0c7cf4b5c19e4d0bb0b111ae8c4fb612d3a",
	    "58df81f838dcb0677d97fe6ccc0a9a25f2d2749336242f70a4074936c88e5fcea8eadc052c74da43db0ff1ab3ea9f94d",
	    "6579f7b73070c0edd8c89dae3b220532b36fde6b1113736260625981a761aa60d08338288ebff4020412aec775a73ccc",
	    "58f184a9634abf60a89b822c6a1b0aa9653a309615fc4667e512bbeb9a74a12fca7ccde35bcf7b166e9f4942884a2350",
	    "9a8c6888740f0f12c47b55437f5d52d4d28eba1d61e91e4c74d0670e1f514de295f8154aea4ce4135e5e8106cc3fe825",
	    "5f3ba5ead75512e2daa8394ebe558995b67a91ac79fd73216fc53b59bde72523221631518e896e9794d01b92829418a5",
	    "4a57c867b3b395475e0a9c886d66b2ac37c6c027aad8a0a6478df9238064f1270caf2dc5e372f1908c26a535c16ee08b",
	    "827064b749505b49411719b1c16c05b826b6bb7f57c8e747f2a3d2acd36ba1ad39c20ca66f2dbe55afcd1c7fd8012f31",
	    "c9efcbaa1ea651763d2d6ac9ca7a76bbd5dc394776293d25a629d0369c415cf1a9326b593687ca56834fa915d7b07c9b",
	    "6663167ee505014100c2ebb6c4cc30691c443edef3166e2def9f95f156d50e9925a79eb1d2badb290eb3d4d9b9847cd4",
	    "74e179e932a9dbce61e0908e6b218736576eca81cd4566b28852a4052f16697cb68dbdcc44c7f92d5ba4fcc1acb289b2",
	    "c435d0db0b67750a29d98b81917eb7835ed9719ad1d019cf80011bf02eed0db7a5db78b7ddbd6272e7e07bb64b3ecc39",
	    "84178a39de10fa688c8c8206dee749c9d77654931996341c9a0d9c8f40d3356e9b89d06636f7675ae9f5aa5a36d664fc",
	    "c691f50ecdb9cd3fb39b41eb56b0617c0cd5c8675581afbc5c964c59656c990d99bad1cc341096b8281d2568a7d36d7c",
	    "fd6425ed1c1022de6d868794e4e0b00e2e984279c9776d3d0916eee000d84977ca218e4c44eb698c8594b00c0686422a",
	    "4caa70e6e2c77af98378dc038e8e8af63bc5213d489b148960b372450cca820684811e1319f17e6b40088661ffc4686e"
	};
	const char *result_hdigest512[] = {
	    "b936cee86c9f87aa5d3c6f2e84cb5a4239a5fe50480a6ec66b70ab5b1f4ac6730c6c515421b327ec1d69402e53dfb49ad7381eb067b338fd7b0cb22247225d47",
	    "fc8c80e6b943cd07eccecf01bc6038bae68ebb6fa2e1e62b44753d7c177af7a46b089df349a19f7622a22312c76906ca9c984e1446d3ab86a98fdfa1425341c5",
	    "1a97e05c35e6727690dfdf2e8079b34fefabf15236abc9170dccdcf5623e4c5ce72a446842bd7607186c9e3f21c0a0edf6ab6c5ec8304a1f969c20c1455e9b7c",
	    "d976d7606862af60e9818754d563bd63481dcd21ff562318a146865eb188736bf337f0295e81e92268381bccac44381ff8ca9254b3a9451785f5f57a7407aa35",
	    "0204fdb2ef5ea614c203f020f6e5d467262f8c382928ede2ab6caf444632951f2472287e2fbf837463771e1641b163dae40ec39078c6c423cac445c3cc52cbce",
	    "68aa0817663801d78ff29dd6ad18bf910d8d6eb56e57b1a055e66a29d32af660f80dfab5adc5012b2fb7c3c8fd0a20c7e8bd07fac69089e5795d405bc5b53353",
	    "d75a832599900737e1b1ac33a50c2451f00de527256892d451e3e40bab342690a5ae84ba4dc75afaac784e747531627e131dcf83a52f3125885c2f844951eb4d",
	    "724a92113a0dea095a688be4b14d26554804a7968c6959ca0cf45bb66ca6804efc8415576e79db69410e39ac7ea0ed41e95557ca6b4183e7f6f0853f5f7a5c86",
	    "d2e482f1c83bb765e0015e48a5c6bb495b6e5124d167773df79792a64b0680009e88ac7476066bf3256eacb31d08989a06d4486dd0af1de02a16458a687d2867",
	    "66fd04be4cff69141410f3ded6e7962135fa8c3cfce976c236a958ab79b39c76017d63d2aaf5c42ead194bece106ed14b9e4e9717292f7aeeb4a2f6dd97e426c",
	    "46c72a64771f5dcdc53d93fbd45564a18e5288ab7dff1d037b82663c3eafb029f002fe6b7bbb52c3ed88d4d143265c6a0741e504674d05b48c32a7b1317b197c",
	    "9f2f0fa68aa6561005e91367e532d7ad7be4feded51fc3fdae881b7d5ef86b751e20c433838bf9a118672c3adcb08991d3933dbbdddbc16d4adb5fec77b77806",
	    "c07737c5c98a8f27306cd3f79fd8e9f5bc6241448e57333140afd162daca60cd9000af9e706ff315346d644b9b2681d8e4d9363e7c06cbe1537bd8d188b872f2",
	    "309fe8aed861ce624895881e337eaf89eb90492d0172b9703f3990f0f19fd079b1bd89ae65e01aaa77613a7bd3ec5e6e936e8d9b60692bec6327ad78dd99f455",
	    "bfa479d5ab67ea22c4fbd4c44f65fdcdcea92d6298698eba4f8844e460deb74a45093cd5cf293b1bf779508836eaa46d29852817375bae1bb98c65ecab627759",
	    "23668222460425cdf11484c2730683518565f525ff984ab1e184578a19f1bee8af0b491fadaff69edfcf25b90d477272616b551868c7895bf8c289addc7cd905",
	    "cb432c15fd4bbc23444b9ba83ce9023fa03de00b539d9e9ae10bbfcedbeae1354cd30dd53f412c1a5874633a6ca573b39f5f800bc5a9599d0b2ed85c3548cee0",
	    "38f1c9eab3731f967a4511b166d859dd11a94ad00b0407888ce8a13a617f36b7d75563ac6d25826af184b7f1699efbabfe5525dde4b4effe1fc60e4b05a7da4e",
	    "85f64cae9f9c457aa2d921c8d0ebd25f07514d034084ee0c5e937097ef98e697f87083231aa738f378e330ce2d4ff533d31e4ca399f99051acff18b3e2451458"
	};

	/* Hash test. */
	for (i = 0; i < nitems(data); i ++) {
		/* 224 */
		sha2_get_digest_str(224, data[i], data_size[i], digest_str, NULL);
		if (0 != memcmp(digest_str, result_digest224[i], SHA2_224_HASH_STR_SIZE))
			return (1);
		/* 256 */
		sha2_get_digest_str(256, data[i], data_size[i], digest_str, NULL);
		if (0 != memcmp(digest_str, result_digest256[i], SHA2_256_HASH_STR_SIZE))
			return (2);
		/* 384 */
		sha2_get_digest_str(384, data[i], data_size[i], digest_str, NULL);
		if (0 != memcmp(digest_str, result_digest384[i], SHA2_384_HASH_STR_SIZE))
			return (3);
		/* 512 */
		sha2_get_digest_str(512, data[i], data_size[i], digest_str, NULL);
		if (0 != memcmp(digest_str, result_digest512[i], SHA2_512_HASH_STR_SIZE))
			return (4);
	}

	/* HMAC test. */
	for (i = 0; i < nitems(data); i ++) {
		/* 256 */
		sha2_hmac_get_digest_str(256, data[i], data_size[i], data[i],
		    data_size[i], digest_str, NULL);
		if (0 != memcmp(digest_str, result_hdigest256[i], SHA2_256_HASH_STR_SIZE))
			return (5);
		/* 384 */
		sha2_hmac_get_digest_str(384, data[i], data_size[i], data[i],
		    data_size[i], digest_str, NULL);
		if (0 != memcmp(digest_str, result_hdigest384[i], SHA2_384_HASH_STR_SIZE))
			return (6);
		/* 512 */
		sha2_hmac_get_digest_str(512, data[i], data_size[i], data[i],
		    data_size[i], digest_str, NULL);
		if (0 != memcmp(digest_str, result_hdigest512[i], SHA2_512_HASH_STR_SIZE))
			return (7);
	}

	return (0);
}
#endif


#endif /* __SHA2_H__INCLUDED__ */
