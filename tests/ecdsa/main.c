#include <sys/param.h>

#ifdef __linux__ /* Linux specific code. */
#	define _GNU_SOURCE /* See feature_test_macros(7) */
#	define __USE_GNU 1
#endif /* Linux specific code. */

#include <sys/types.h>
#include <sys/time.h> /* For getrusage. */
#include <sys/resource.h>
#include <sys/sysctl.h>
     
#include <inttypes.h>
#include <stdlib.h> /* malloc, exit */
#include <stdio.h> /* snprintf, fprintf */
#include <unistd.h> /* close, write, sysconf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strerror... */
#include <time.h>
#include <errno.h>
#include <err.h>


#define BN_DIGIT_BIT_CNT 	64
#define BN_BIT_LEN		1408
#define BN_CC_MULL_DIV		1
#define BN_NO_POINTERS_CHK	1
#define BN_MOD_REDUCE_ALGO	BN_MOD_REDUCE_ALGO_BASIC
#define BN_SELF_TEST		1
#define EC_USE_PROJECTIVE	1
#define EC_PROJ_REPEAT_DOUBLE	1
#define EC_PROJ_ADD_MIX		1
#define EC_PF_FXP_MULT_ALGO	EC_PF_FXP_MULT_ALGO_COMB_2T
#define EC_PF_FXP_MULT_WIN_BITS	9
#define EC_PF_UNKPT_MULT_ALGO	EC_PF_UNKPT_MULT_ALGO_COMB_1T
#define EC_PF_UNKPT_MULT_WIN_BITS 2
#define EC_PF_TWIN_MULT_ALGO	EC_PF_TWIN_MULT_ALGO_INTER //EC_PF_TWIN_MULT_ALGO_JOINT //EC_PF_TWIN_MULT_ALGO_FXP_UNKPT
#define EC_DISABLE_PUB_KEY_CHK	1
#define EC_SELF_TEST		1

#include "crypto/dsa/ecdsa.h"


#define LOG_INFO_FMT(fmt, args...)					\
	    fprintf(stdout, fmt"\n", ##args)


int
main(int argc, char *argv[]) {
	int error;

	error = bn_self_test();
	if (0 != error) {
		LOG_INFO_FMT("bn_self_test(): err: %i", error);
	}

	error = ec_self_test();
	if (0 != error) {
		LOG_INFO_FMT("ec_self_test(): err: %i", error);
	}

	return (error);
}
