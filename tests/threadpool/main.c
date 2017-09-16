/*-
 * Copyright (c) 2016 - 2017 Rozhuk Ivan <rozhuk.im@gmail.com>
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
#define _GNU_SOURCE /* See feature_test_macros(7) */
#define __USE_GNU 1
#endif /* Linux specific code. */

#include <sys/types.h>
#include <sys/time.h> /* For getrusage. */
#include <sys/resource.h>
#include <sys/fcntl.h> /* open, fcntl */

#include <inttypes.h>
#include <stdlib.h> /* malloc, exit */
#include <stdio.h> /* snprintf, fprintf */
#include <unistd.h> /* close, write, sysconf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strerror... */
#include <errno.h>
#include <err.h>

#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#include "threadpool/threadpool.h"
#include "threadpool/threadpool_msg_sys.h"


#undef PACKAGE_NAME
#define PACKAGE_NAME	"test core_thrp"

#if 0
#	include "core_log.h"
#else
#	define LOG_INFO(fmt)			fprintf(stdout, fmt"\n")
#	define LOG_INFO_FMT(fmt, args...)	fprintf(stdout, fmt"\n", ##args)
	static uintptr_t core_log_fd;
#endif


#define THREADS_COUNT_MAX		16
#define TEST_EV_CNT_MAX			12
#define TEST_TIMER_ID			36434632 /* Random value. */
#define TEST_TIMER_INTERVAL		14
#define TEST_SLEEP_TIME_NS		500000000
#ifdef DEBUG__
#	define TEST_SLEEP_TIME_S	5
#else
#	define TEST_SLEEP_TIME_S	0
#endif

static thrp_p 	thrp = NULL;
static size_t	threads_count;
static int 	pipe_fd[2] = {-1, -1};
static uint8_t	thr_arr[(THREADS_COUNT_MAX + 4)];

static int	init_suite(void);
static int	clean_suite(void);

static void	test_thrp_init1(void);
static void	test_thrp_init16(void);
static void	test_thrp_destroy(void);
static void	test_thrp_threads_create(void);
static void	test_thrp_thread_count_max_get(void);
static void	test_thrp_thread_count_get(void);
static void	test_thrp_thread_get(void);
static void	test_thrp_thread_get_current(void);
static void	test_thrp_thread_get_rr(void);
static void	test_thrp_thread_get_pvt(void);
static void	test_thrp_thread_get_cpu_id(void);
static void	test_thrpt_get_thrp(void);
static void	test_thrpt_msg_send(void);
static void	test_thrpt_msg_bsend_ex1(void);
static void	test_thrpt_msg_bsend_ex2(void);
static void	test_thrpt_msg_bsend_ex3(void);
static void	test_thrpt_msg_cbsend1(void);
static void	test_thrpt_msg_cbsend2(void);


static void	test_thrpt_ev_add_ex_rd_0(void);
static void	test_thrpt_ev_add_ex_rd_oneshot(void);
static void	test_thrpt_ev_add_ex_rd_dispatch(void);
static void	test_thrpt_ev_add_ex_rd_edge(void);

static void	test_thrpt_ev_add_ex_rw_0(void);
static void	test_thrpt_ev_add_ex_rw_oneshot(void);
static void	test_thrpt_ev_add_ex_rw_dispatch(void);
static void	test_thrpt_ev_add_ex_rw_edge(void);

static void	test_thrpt_ev_add_ex_tmr_0(void);
static void	test_thrpt_ev_add_ex_tmr_oneshot(void);
static void	test_thrpt_ev_add_ex_tmr_dispatch(void);
static void	test_thrpt_ev_add_ex_tmr_edge(void);
static void	test_thrpt_gettimev(void);



int
main(int argc, char *argv[]) {
	CU_pSuite psuite = NULL;

	core_log_fd = (uintptr_t)open("/dev/stdout", (O_WRONLY | O_APPEND));

	LOG_INFO("\n\n");
	LOG_INFO(PACKAGE_NAME": started");
#ifdef DEBUG
	LOG_INFO("Build: "__DATE__" "__TIME__", DEBUG");
#else
	LOG_INFO("Build: "__DATE__" "__TIME__", Release");
#endif

	setpriority(PRIO_PROCESS, 0, -20);

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return (CU_get_error());
	/* add a suite to the registry */
	psuite = CU_add_suite("Core Thread Poll", init_suite, clean_suite);
	if (NULL == psuite) {
		CU_cleanup_registry();
		return (CU_get_error());
	}

	/* add the tests to the suite */
	if (NULL == CU_add_test(psuite, "test of test_thrp_init1() - threads count = 1", test_thrp_init1) ||
	    NULL == CU_add_test(psuite, "test of thrp_threads_create()", test_thrp_threads_create) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_count_max_get()", test_thrp_thread_count_max_get) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_count_get()", test_thrp_thread_count_get) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get()", test_thrp_thread_get) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_current()", test_thrp_thread_get_current) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_rr()", test_thrp_thread_get_rr) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_pvt()", test_thrp_thread_get_pvt) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_cpu_id()", test_thrp_thread_get_cpu_id) ||
	    NULL == CU_add_test(psuite, "test of thrpt_get_thrp()", test_thrpt_get_thrp) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_send()", test_thrpt_msg_send) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_bsend_ex(0)", test_thrpt_msg_bsend_ex1) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_bsend_ex(THRP_BMSG_F_SYNC)", test_thrpt_msg_bsend_ex2) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_bsend_ex((THRP_BMSG_F_SYNC | THRP_BMSG_F_SYNC_USLEEP))", test_thrpt_msg_bsend_ex3) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_cbsend(0)", test_thrpt_msg_cbsend1) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_cbsend(THRP_CBMSG_F_ONE_BY_ONE)", test_thrpt_msg_cbsend2) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, 0)", test_thrpt_ev_add_ex_rd_0) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, THRP_F_ONESHOT)", test_thrpt_ev_add_ex_rd_oneshot) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, THRP_F_DISPATCH)", test_thrpt_ev_add_ex_rd_dispatch) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, THRP_F_EDGE)", test_thrpt_ev_add_ex_rd_edge) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, 0)", test_thrpt_ev_add_ex_rw_0) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, THRP_F_ONESHOT)", test_thrpt_ev_add_ex_rw_oneshot) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, THRP_F_DISPATCH)", test_thrpt_ev_add_ex_rw_dispatch) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, THRP_F_EDGE)", test_thrpt_ev_add_ex_rw_edge) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, 0)", test_thrpt_ev_add_ex_tmr_0) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, THRP_F_ONESHOT)", test_thrpt_ev_add_ex_tmr_oneshot) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, THRP_F_DISPATCH)", test_thrpt_ev_add_ex_tmr_dispatch) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, THRP_F_EDGE)", test_thrpt_ev_add_ex_tmr_edge) ||
	    NULL == CU_add_test(psuite, "test of thrpt_gettimev()", test_thrpt_gettimev) ||
	    NULL == CU_add_test(psuite, "test of test_thrp_destroy()", test_thrp_destroy) ||
	    0 ||
	    NULL == CU_add_test(psuite, "test of test_thrp_init16() - threads count = 16", test_thrp_init16) ||
	    NULL == CU_add_test(psuite, "test of thrp_threads_create()", test_thrp_threads_create) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_count_max_get()", test_thrp_thread_count_max_get) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_count_get()", test_thrp_thread_count_get) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get()", test_thrp_thread_get) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_current()", test_thrp_thread_get_current) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_rr()", test_thrp_thread_get_rr) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_pvt()", test_thrp_thread_get_pvt) ||
	    NULL == CU_add_test(psuite, "test of thrp_thread_get_cpu_id()", test_thrp_thread_get_cpu_id) ||
	    NULL == CU_add_test(psuite, "test of thrpt_get_thrp()", test_thrpt_get_thrp) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_send()", test_thrpt_msg_send) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_bsend_ex(0)", test_thrpt_msg_bsend_ex1) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_bsend_ex(THRP_BMSG_F_SYNC)", test_thrpt_msg_bsend_ex2) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_bsend_ex((THRP_BMSG_F_SYNC | THRP_BMSG_F_SYNC_USLEEP))", test_thrpt_msg_bsend_ex3) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_cbsend(0)", test_thrpt_msg_cbsend1) ||
	    NULL == CU_add_test(psuite, "test of thrpt_msg_cbsend(THRP_CBMSG_F_ONE_BY_ONE)", test_thrpt_msg_cbsend2) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, 0)", test_thrpt_ev_add_ex_rd_0) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, THRP_F_ONESHOT)", test_thrpt_ev_add_ex_rd_oneshot) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, THRP_F_DISPATCH)", test_thrpt_ev_add_ex_rd_dispatch) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_READ, THRP_F_EDGE)", test_thrpt_ev_add_ex_rd_edge) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, 0)", test_thrpt_ev_add_ex_rw_0) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, THRP_F_ONESHOT)", test_thrpt_ev_add_ex_rw_oneshot) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, THRP_F_DISPATCH)", test_thrpt_ev_add_ex_rw_dispatch) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_WRITE, THRP_F_EDGE)", test_thrpt_ev_add_ex_rw_edge) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, 0)", test_thrpt_ev_add_ex_tmr_0) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, THRP_F_ONESHOT)", test_thrpt_ev_add_ex_tmr_oneshot) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, THRP_F_DISPATCH)", test_thrpt_ev_add_ex_tmr_dispatch) ||
	    NULL == CU_add_test(psuite, "test of thrpt_ev_add_ex(THRP_EV_TIMER, THRP_F_EDGE)", test_thrpt_ev_add_ex_tmr_edge) ||
	    NULL == CU_add_test(psuite, "test of thrpt_gettimev()", test_thrpt_gettimev) ||
	    NULL == CU_add_test(psuite, "test of test_thrp_destroy()", test_thrp_destroy) ||
	    0) {
		CU_cleanup_registry();
		return (CU_get_error());
	}

	/* Run all tests using the basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	printf("\n");
	CU_basic_show_failures(CU_get_failure_list());
	printf("\n\n");

	/* Run all tests using the automated interface */
	CU_automated_run_tests();
	CU_list_tests_to_file();

	/* Clean up registry and return */
	CU_cleanup_registry();

	return (CU_get_error());
}


static void
test_sleep(time_t sec, long nsec) { /* 1 sec = 1000000000 nanoseconds */
	struct timespec rqtp;

	rqtp.tv_sec = sec;
	rqtp.tv_nsec = nsec;
	for (;;) {
		if (0 == nanosleep(&rqtp, &rqtp) ||
		    EINTR != errno)
			break;
	}
}


static int
init_suite(void) {
	int error;

	error = thrp_init();
	if (0 != error)
		return (error);
	if (-1 == pipe2(pipe_fd, O_NONBLOCK))
		return (errno);
	return (0);
}

static int
clean_suite(void) {

	close(pipe_fd[0]);
	close(pipe_fd[1]);
	return (0);
}



static void
test_thrp_init1(void) {
	int error;
	thrp_settings_t s;

	thrp_def_settings(&s);
	threads_count = 1;
	s.threads_max = 1;
	s.flags = (THRP_S_F_BIND2CPU | THRP_S_F_CACHE_TIME_SYSC);
	error = thrp_create(&s, &thrp);
	CU_ASSERT(0 == error);
	if (0 != error)
		return;
	/* Wait for all threads start. */
	test_sleep(1, 0);
}

static void
test_thrp_destroy(void) {

	thrp_shutdown(thrp);
	thrp_shutdown_wait(thrp);
	thrp_destroy(thrp);
	thrp = NULL;
	CU_PASS("thrp_destroy()");
}

static void
test_thrp_init16(void) {
	int error;
	thrp_settings_t s;

	thrp_def_settings(&s);
	threads_count = THREADS_COUNT_MAX;
	s.threads_max = THREADS_COUNT_MAX;
	s.flags = (THRP_S_F_BIND2CPU | THRP_S_F_CACHE_TIME_SYSC);
	error = thrp_create(&s, &thrp);
	CU_ASSERT(0 == error);
	if (0 != error)
		return;
	/* Wait for all threads start. */
	test_sleep(1, 0);
}

static void
test_thrp_threads_create(void) {

	CU_ASSERT(0 == thrp_threads_create(thrp, 0));
}

static void
test_thrp_thread_count_max_get(void) {

	CU_ASSERT(threads_count == thrp_thread_count_max_get(thrp));
}

static void
test_thrp_thread_count_get(void) {

	CU_ASSERT(threads_count == thrp_thread_count_get(thrp));
}

static void
test_thrp_thread_get(void) {
	size_t i;

	for (i = 0; i < threads_count; i ++) {
		if (i != thrp_thread_get_num(thrp_thread_get(thrp, i))) {
			CU_FAIL("thrp_thread_get_num()");
			return; /* Fail. */
		}
	}
	CU_PASS("thrp_thread_get_num()");
}

static void
test_thrp_thread_get_current(void) {

	CU_ASSERT(NULL == thrp_thread_get_current());
}

static void
test_thrp_thread_get_rr(void) {

	CU_ASSERT(NULL != thrp_thread_get_rr(thrp));
}

static void
test_thrp_thread_get_pvt(void) {

	CU_ASSERT(NULL != thrp_thread_get_pvt(thrp));
}

static void
test_thrp_thread_get_cpu_id(void) {

	CU_ASSERT(0 == thrp_thread_get_cpu_id(thrp_thread_get(thrp, 0)));
}

static void
test_thrpt_get_thrp(void) {

	CU_ASSERT(thrp == thrpt_get_thrp(thrp_thread_get(thrp, 0)));
}



static void
msg_send_cb(thrpt_p thrpt, void *udata) {

	CU_ASSERT((size_t)udata == thrp_thread_get_num(thrpt));

	if ((size_t)udata == thrp_thread_get_num(thrpt)) {
		thr_arr[(size_t)udata] = (((size_t)udata) & 0xff);
	}
}
static void
test_thrpt_msg_send(void) {
	size_t i;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	for (i = 0; i < threads_count; i ++) {
		if (0 != thrpt_msg_send(thrp_thread_get(thrp, i), NULL,
		    0, msg_send_cb, (void*)i)) {
			CU_FAIL("thrpt_msg_send()");
			return; /* Fail. */
		}
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_S, TEST_SLEEP_TIME_NS);
	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("thrpt_msg_send() - not work.");
			return; /* Fail. */
		}
	}
	CU_PASS("thrpt_msg_send()");
}

static void
msg_bsend_cb(thrpt_p thrpt, void *udata) {

	CU_ASSERT(udata == (void*)thrpt_get_thrp(thrpt));

	if (udata == (void*)thrpt_get_thrp(thrpt)) {
		thr_arr[thrp_thread_get_num(thrpt)] = (uint8_t)thrp_thread_get_num(thrpt);
	} else {
		thr_arr[thrp_thread_get_num(thrpt)] = 0xff;
	}
}
static void
test_thrpt_msg_bsend_ex1(void) {
	size_t i;
	size_t send_msg_cnt, error_cnt;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != thrpt_msg_bsend_ex(thrp, NULL, 0, msg_bsend_cb,
	    (void*)thrp, &send_msg_cnt, &error_cnt)) {
		CU_FAIL("thrpt_msg_bsend_ex()");
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_S, TEST_SLEEP_TIME_NS);

	if (threads_count != send_msg_cnt ||
	    0 != error_cnt) {
		CU_FAIL("thrpt_msg_bsend_ex() - not all received.");
		return; /* Fail. */
	}

	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("thrpt_msg_bsend_ex() - not work.");
			return; /* Fail. */
		}
	}
	CU_PASS("thrpt_msg_bsend_ex()");
}
static void
test_thrpt_msg_bsend_ex2(void) {
	size_t i;
	size_t send_msg_cnt, error_cnt;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != thrpt_msg_bsend_ex(thrp, NULL, THRP_BMSG_F_SYNC, msg_bsend_cb,
	    (void*)thrp, &send_msg_cnt, &error_cnt)) {
		CU_FAIL("thrpt_msg_bsend_ex(THRP_BMSG_F_SYNC)");
		return; /* Fail. */
	}

	if (threads_count != send_msg_cnt ||
	    0 != error_cnt) {
		CU_FAIL("thrpt_msg_bsend_ex(THRP_BMSG_F_SYNC) - not all received.");
		return; /* Fail. */
	}

	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("thrpt_msg_bsend_ex(THRP_BMSG_F_SYNC) - not work.");
			return; /* Fail. */
		}
	}
	CU_PASS("thrpt_msg_bsend_ex(THRP_BMSG_F_SYNC)");
}
static void
test_thrpt_msg_bsend_ex3(void) {
	size_t i;
	size_t send_msg_cnt, error_cnt;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != thrpt_msg_bsend_ex(thrp, NULL,
	    (THRP_BMSG_F_SYNC | THRP_BMSG_F_SYNC_USLEEP), msg_bsend_cb,
	    (void*)thrp, &send_msg_cnt, &error_cnt)) {
		CU_FAIL("thrpt_msg_bsend_ex((THRP_BMSG_F_SYNC | THRP_BMSG_F_SYNC_USLEEP))");
		return; /* Fail. */
	}

	if (threads_count != send_msg_cnt ||
	    0 != error_cnt) {
		CU_FAIL("thrpt_msg_bsend_ex((THRP_BMSG_F_SYNC | THRP_BMSG_F_SYNC_USLEEP)) - not all received.");
		return; /* Fail. */
	}

	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("thrpt_msg_bsend_ex((THRP_BMSG_F_SYNC | THRP_BMSG_F_SYNC_USLEEP)) - not work.");
			return; /* Fail. */
		}
	}
	CU_PASS("thrpt_msg_bsend_ex((THRP_BMSG_F_SYNC | THRP_BMSG_F_SYNC_USLEEP))");
}

static void
msg_cbsend_cb(thrpt_p thrpt, void *udata) {

	CU_ASSERT(udata == (void*)thrpt_get_thrp(thrpt));

	if (udata == (void*)thrpt_get_thrp(thrpt)) {
		thr_arr[thrp_thread_get_num(thrpt)] = (uint8_t)thrp_thread_get_num(thrpt);
	}
}
static void
msg_cbsend_done_cb(thrpt_p thrpt, size_t send_msg_cnt,
    size_t error_cnt, void *udata) {

	CU_ASSERT(udata == (void*)thrpt_get_thrp(thrpt));
	CU_ASSERT(threads_count == send_msg_cnt);
	CU_ASSERT(0 == error_cnt);

	if (udata == (void*)thrpt_get_thrp(thrpt) &&
	    threads_count == send_msg_cnt &&
	    0 == error_cnt) {
		thr_arr[threads_count] = (uint8_t)threads_count;
	}
}
static void
test_thrpt_msg_cbsend1(void) {
	size_t i;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != thrpt_msg_cbsend(thrp, thrp_thread_get(thrp, 0),
	    0, msg_cbsend_cb, (void*)thrp, msg_cbsend_done_cb)) {
		CU_FAIL("thrpt_msg_cbsend()");
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_S, TEST_SLEEP_TIME_NS);
	for (i = 0; i < (threads_count + 1); i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("thrpt_msg_cbsend() - not work.");
			return; /* Fail. */
		}
	}
	CU_PASS("thrpt_msg_cbsend()");
}
static void
test_thrpt_msg_cbsend2(void) {
	size_t i;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != thrpt_msg_cbsend(thrp, thrp_thread_get(thrp, 0),
	    THRP_CBMSG_F_ONE_BY_ONE, msg_cbsend_cb, (void*)thrp,
	    msg_cbsend_done_cb)) {
		CU_FAIL("thrpt_msg_cbsend(THRP_CBMSG_F_ONE_BY_ONE)");
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_S, TEST_SLEEP_TIME_NS);
	for (i = 0; i < (threads_count + 1); i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("thrpt_msg_cbsend(THRP_CBMSG_F_ONE_BY_ONE) - not work.");
			return; /* Fail. */
		}
	}
	CU_PASS("thrpt_msg_cbsend(THRP_CBMSG_F_ONE_BY_ONE)");
}


static void
thrpt_ev_add_r_cb(thrp_event_p ev, thrp_udata_p thrp_udata) {

	CU_ASSERT(0 != ev->data);
	CU_ASSERT(THRP_EV_READ == ev->event);
	CU_ASSERT(thrpt_ev_add_r_cb == thrp_udata->cb_func);
	CU_ASSERT(pipe_fd[0] == (int)thrp_udata->ident);

	//read(pipe_fd[0], buf, sizeof(buf));
	if (0 != ev->data &&
	    THRP_EV_READ == ev->event &&
	    thrpt_ev_add_r_cb == thrp_udata->cb_func &&
	    pipe_fd[0] == (int)thrp_udata->ident) {
		thr_arr[0] ++;
		if (TEST_EV_CNT_MAX <= thr_arr[0]) {
			thrpt_ev_enable(0, THRP_EV_READ, thrp_udata);
		}
	}
}
static void
test_thrpt_ev_add_ex_rd(uint16_t flags, uint8_t res, int remove_ok) {
	thrp_udata_t thrp_udata;
	uint8_t buf[(TEST_EV_CNT_MAX * 2)];

	/* Init. */
	thr_arr[0] = 0;
	memset(&thrp_udata, 0x00, sizeof(thrp_udata));
	read(pipe_fd[0], buf, sizeof(buf));

	thrp_udata.cb_func = thrpt_ev_add_r_cb;
	thrp_udata.ident = (uintptr_t)pipe_fd[0];
	if (0 != thrpt_ev_add_ex(thrp_thread_get(thrp, 0), THRP_EV_READ,
	    flags, 0, 0, &thrp_udata)) {
		CU_FAIL("thrpt_ev_add_ex(THRP_EV_READ)"); /* Fail. */
		read(pipe_fd[0], buf, sizeof(buf));
		thrpt_ev_del(THRP_EV_READ, &thrp_udata);
		return; /* Fail. */
	}
	CU_ASSERT(1 == write(pipe_fd[1], "1", 1));
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_S, TEST_SLEEP_TIME_NS);
	if (res != thr_arr[0]) {
		CU_FAIL("thrpt_ev_add_ex(THRP_EV_READ) - not work"); /* Fail. */
		LOG_INFO_FMT("%i", (int)thr_arr[0]);
	}
	/* Clean. */
	read(pipe_fd[0], buf, sizeof(buf));
	if (0 != remove_ok) {
		CU_ASSERT(0 == thrpt_ev_del(THRP_EV_READ, &thrp_udata));
	}
	CU_ASSERT(0 != thrpt_ev_del(THRP_EV_READ, &thrp_udata));
}
static void
test_thrpt_ev_add_ex_rd_0(void) {

	test_thrpt_ev_add_ex_rd(0, TEST_EV_CNT_MAX, 1);
}
static void
test_thrpt_ev_add_ex_rd_oneshot(void) {

	test_thrpt_ev_add_ex_rd(THRP_F_ONESHOT, 1, 0);
}
static void
test_thrpt_ev_add_ex_rd_dispatch(void) {

	test_thrpt_ev_add_ex_rd(THRP_F_DISPATCH, 1, 1);
}
static void
test_thrpt_ev_add_ex_rd_edge(void) {

	test_thrpt_ev_add_ex_rd(THRP_F_EDGE, 1, 1);
}


static void
thrpt_ev_add_w_cb(thrp_event_p ev, thrp_udata_p thrp_udata) {

	CU_ASSERT(0 != ev->data);
	CU_ASSERT(THRP_EV_WRITE == ev->event);
	CU_ASSERT(thrpt_ev_add_w_cb == thrp_udata->cb_func);
	CU_ASSERT(pipe_fd[1] == (int)thrp_udata->ident);

	if (0 != ev->data &&
	    THRP_EV_WRITE == ev->event &&
	    thrpt_ev_add_w_cb == thrp_udata->cb_func &&
	    pipe_fd[1] == (int)thrp_udata->ident &&
	    1 == write(pipe_fd[1], "1", 1) &&
	    1 == write(pipe_fd[1], "1", 1)) { /* Dup for THRP_F_ONESHOT test. */
		thr_arr[0] ++;
		if (TEST_EV_CNT_MAX <= thr_arr[0]) {
			thrpt_ev_enable(0, THRP_EV_WRITE, thrp_udata);
		}
	}
}
static void
test_thrpt_ev_add_ex_rw(uint16_t flags, uint8_t res, int remove_ok) {
	thrp_udata_t thrp_udata;
	uint8_t buf[(TEST_EV_CNT_MAX * 2)];

	/* Init. */
	thr_arr[0] = 0;
	memset(&thrp_udata, 0x00, sizeof(thrp_udata));
	read(pipe_fd[0], buf, sizeof(buf));

	thrp_udata.cb_func = thrpt_ev_add_w_cb;
	thrp_udata.ident = (uintptr_t)pipe_fd[1];
	if (0 != thrpt_ev_add_ex(thrp_thread_get(thrp, 0), THRP_EV_WRITE,
	    flags, 0, 0, &thrp_udata)) {
		CU_FAIL("thrpt_ev_add_ex(THRP_EV_WRITE)"); /* Fail. */
		read(pipe_fd[0], buf, sizeof(buf));
		thrpt_ev_del(THRP_EV_WRITE, &thrp_udata);
		return; /* Fail. */
	}
	CU_ASSERT(1 == write(pipe_fd[1], "1", 1));
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_S, TEST_SLEEP_TIME_NS);
	if (res != thr_arr[0]) {
		CU_FAIL("thrpt_ev_add_ex(THRP_EV_WRITE) - not work"); /* Fail. */
		LOG_INFO_FMT("%i", (int)thr_arr[0]);
	}
	/* Clean. */
	read(pipe_fd[0], buf, sizeof(buf));
	if (0 != remove_ok) {
		CU_ASSERT(0 == thrpt_ev_del(THRP_EV_WRITE, &thrp_udata));
	}
	CU_ASSERT(0 != thrpt_ev_del(THRP_EV_WRITE, &thrp_udata));
}
static void
test_thrpt_ev_add_ex_rw_0(void) {

	test_thrpt_ev_add_ex_rw(0, TEST_EV_CNT_MAX, 1);
}
static void
test_thrpt_ev_add_ex_rw_oneshot(void) {

	test_thrpt_ev_add_ex_rw(THRP_F_ONESHOT, 1, 0);
}
static void
test_thrpt_ev_add_ex_rw_dispatch(void) {

	test_thrpt_ev_add_ex_rw(THRP_F_DISPATCH, 1, 1);
}
static void
test_thrpt_ev_add_ex_rw_edge(void) {

	test_thrpt_ev_add_ex_rw(THRP_F_EDGE, 1, 1);
}


static void
thrpt_ev_add_tmr_cb(thrp_event_p ev, thrp_udata_p thrp_udata) {

	CU_ASSERT(THRP_EV_TIMER == ev->event);
	CU_ASSERT(thrpt_ev_add_tmr_cb == thrp_udata->cb_func);
	CU_ASSERT(TEST_TIMER_ID == thrp_udata->ident);

	if (THRP_EV_TIMER == ev->event &&
	    thrpt_ev_add_tmr_cb == thrp_udata->cb_func &&
	    TEST_TIMER_ID == thrp_udata->ident) {
		thr_arr[0] ++;
		if (TEST_EV_CNT_MAX <= thr_arr[0]) {
			thrpt_ev_enable(0, THRP_EV_TIMER, thrp_udata);
		}
	}
}
static void
test_thrpt_ev_add_ex_tmr(uint16_t flags, uint8_t res, int remove_ok) {
	thrp_udata_t thrp_udata;

	/* Init. */
	thr_arr[0] = 0;
	memset(&thrp_udata, 0x00, sizeof(thrp_udata));

	thrp_udata.cb_func = thrpt_ev_add_tmr_cb;
	thrp_udata.ident = TEST_TIMER_ID;
	if (0 != thrpt_ev_add_ex(thrp_thread_get(thrp, 0), THRP_EV_TIMER,
	    flags, 0, TEST_TIMER_INTERVAL, &thrp_udata)) {
		CU_FAIL("thrpt_ev_add_ex(THRP_EV_TIMER)"); /* Fail. */
		thrpt_ev_del(THRP_EV_TIMER, &thrp_udata);
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep(0, 300000000);
	if (res != thr_arr[0]) {
		CU_FAIL("thrpt_ev_add_ex(THRP_EV_TIMER) - not work"); /* Fail. */
		LOG_INFO_FMT("%i", (int)thr_arr[0]);
	}
	/* Clean. */
	if (0 != remove_ok) {
		CU_ASSERT(0 == thrpt_ev_del(THRP_EV_TIMER, &thrp_udata));
	}
	CU_ASSERT(0 != thrpt_ev_del(THRP_EV_TIMER, &thrp_udata));
}
static void
test_thrpt_ev_add_ex_tmr_0(void) {

	test_thrpt_ev_add_ex_tmr(0, TEST_EV_CNT_MAX, 1);
}
static void
test_thrpt_ev_add_ex_tmr_oneshot(void) {

	test_thrpt_ev_add_ex_tmr(THRP_F_ONESHOT, 1, 0);
}
static void
test_thrpt_ev_add_ex_tmr_dispatch(void) {

	test_thrpt_ev_add_ex_tmr(THRP_F_DISPATCH, 1, 1);
}
static void
test_thrpt_ev_add_ex_tmr_edge(void) {

	test_thrpt_ev_add_ex_tmr(THRP_F_EDGE, TEST_EV_CNT_MAX, 1);
}


static void
test_thrpt_gettimev(void) {
	struct timespec tp0, tp1;
	struct timespec tpr0, tpr1;

	memset(&tp0, 0x00, sizeof(tp0));
	memset(&tp1, 0x00, sizeof(tp1));
	memset(&tpr0, 0x00, sizeof(tpr0));
	memset(&tpr1, 0x00, sizeof(tpr1));

	CU_ASSERT(0 == thrpt_gettimev(thrp_thread_get(thrp, 0), 0, &tp0));
	CU_ASSERT(0 == thrpt_gettimev(thrp_thread_get(thrp, 0), 1, &tpr0));
	/* Wait... */
	test_sleep(1, 0);

	CU_ASSERT(0 == thrpt_gettimev(thrp_thread_get(thrp, 0), 0, &tp1));
	CU_ASSERT(0 == thrpt_gettimev(thrp_thread_get(thrp, 0), 1, &tpr1));

	CU_ASSERT(0 != memcmp(&tp0, &tp1, sizeof(struct timespec)));
	CU_ASSERT(0 != memcmp(&tpr0, &tpr1, sizeof(struct timespec)));
	CU_ASSERT(0 != memcmp(&tp0, &tpr0, sizeof(struct timespec)));
	CU_ASSERT(0 != memcmp(&tp1, &tpr1, sizeof(struct timespec)));
}


#if 0
int	thrp_thread_attach_first(thrp_p thrp);
int	thrp_thread_dettach(thrpt_p thrpt);

int	thrpt_ev_add(thrpt_p thrpt, uint16_t event, uint16_t flags,
	    thrp_udata_p thrp_udata);
int	thrpt_ev_add2(thrpt_p thrpt, thrp_event_p ev, thrp_udata_p thrp_udata);
/*
 * flags - allowed: THRP_F_ONESHOT, THRP_F_DISPATCH, THRP_F_EDGE
 */
int	thrpt_ev_del(uint16_t event, thrp_udata_p thrp_udata);
int	thrpt_ev_enable(int enable, uint16_t event, thrp_udata_p thrp_udata);
int	thrpt_ev_enable_ex(int enable, uint16_t event, uint16_t flags,
	    uint32_t fflags, uint64_t data, thrp_udata_p thrp_udata);

/* Thread cached time functions. */
time_t	thrpt_gettime(thrpt_p thrpt, int real_time);
#endif
