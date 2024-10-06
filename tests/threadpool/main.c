/*-
 * Copyright (c) 2016-2024 Rozhuk Ivan <rozhuk.im@gmail.com>
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

/* Required: devel/cunit */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h> /* For getrusage. */
#include <sys/resource.h>
#include <sys/fcntl.h> /* open, fcntl */

#include <inttypes.h>
#include <stdlib.h> /* malloc, exit */
#include <stdio.h> /* snprintf, fprintf */
#include <unistd.h> /* close, write, sysconf */
#include <string.h> /* memcpy, memmove, memset, strerror... */
#include <errno.h>
#include <err.h>
#include <syslog.h>
#include <spawn.h>
#include <fcntl.h>

#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#include "al/os.h"
#include "threadpool/threadpool.h"
#include "threadpool/threadpool_msg_sys.h"


#undef PACKAGE_NAME
#define PACKAGE_NAME	"test threadpool"

#define LOG_CONS_INFO(fmt)		fprintf(stdout, fmt"\n")
#define LOG_CONS_INFO_FMT(fmt, args...)	fprintf(stdout, fmt"\n", ##args)


#define THREADS_COUNT_MAX		16
#define TEST_EV_CNT_MAX			12
#define TEST_TIMER_ID			36434632 /* Random value. */
#define TEST_TIMER_INTERVAL		30
#define TEST_PROC_INTERVAL		1 /* sec */
#define TEST_PROC_INTERVAL_STR		"1"
#define TEST_SLEEP_TIME_MS		1000

extern char **	environ;
static tp_p 	tp = NULL;
static size_t	threads_count;
static int 	pipe_fd[2] = {-1, -1};
static pid_t	pid;
static uint8_t	thr_arr[(THREADS_COUNT_MAX + 4)];
static uint8_t	thr_tls_arr[(THREADS_COUNT_MAX + 4)];
static size_t	thr_flood_arr[(THREADS_COUNT_MAX + 4)];

static int	init_suite(void);
static int	clean_suite(void);

static void	test_tp_init1(void);
static void	test_tp_init16(void);
static void	test_tp_destroy(void);
static void	test_tp_tpt_hooks(void);
static void	test_tp_threads_create(void);
static void	test_tp_udata_get(void);
static void	test_tp_udata_set(void);
static void	test_tp_thread_count_max_get(void);
static void	test_tp_thread_count_get(void);
static void	test_tp_thread_is_tp_thr(void);
static void	test_tp_thread_get(void);
static void	test_tp_thread_get_rr(void);
static void	test_tp_thread_get_pvt(void);
static void	test_tpt_get_current(void);
static void	test_tpt_get_cpu_id(void);
static void	test_tpt_get_tp(void);
static void	test_tpt_get_msg_queue(void);
static void	test_tpt_msg_send(void);
static void	test_tpt_msg_bsend_ex1(void);
static void	test_tpt_msg_bsend_ex2(void);
static void	test_tpt_msg_bsend_ex3(void);
static void	test_tpt_msg_cbsend1(void);
static void	test_tpt_msg_cbsend2(void);


static void	test_tpt_ev_add_ex_rd_0(void);
static void	test_tpt_ev_add_ex_rd_oneshot(void);
static void	test_tpt_ev_add_ex_rd_dispatch(void);
#ifdef TP_F_EDGE
static void	test_tpt_ev_add_ex_rd_edge(void);
#endif

static void	test_tpt_ev_add_ex_rw_0(void);
static void	test_tpt_ev_add_ex_rw_oneshot(void);
static void	test_tpt_ev_add_ex_rw_dispatch(void);
#ifdef TP_F_EDGE
static void	test_tpt_ev_add_ex_rw_edge(void);
#endif

static void	test_tpt_ev_add_ex_tmr_0(void);
static void	test_tpt_ev_add_ex_tmr_oneshot(void);
static void	test_tpt_ev_add_ex_tmr_dispatch(void);
#ifdef TP_F_EDGE
static void	test_tpt_ev_add_ex_tmr_edge(void);
#endif
static void	test_tpt_ev_add_ex_proc_0(void);
static void	test_tp_pvt_msg_flood(void);



int
main(int argc __unused, char *argv[] __unused) {
	int error = 0;
	CU_pSuite psuite = NULL;

	openlog(PACKAGE_NAME, (LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID), LOG_USER);
	setlogmask(LOG_UPTO(LOG_DEBUG));

	LOG_CONS_INFO("\n\n");
	LOG_CONS_INFO(PACKAGE_NAME": started!");
#ifdef DEBUG
	LOG_CONS_INFO("Build: "__DATE__" "__TIME__", DEBUG");
#else
	LOG_CONS_INFO("Build: "__DATE__" "__TIME__", Release");
#endif

	setpriority(PRIO_PROCESS, 0, -20);

	/* Initialize the CUnit test registry. */
	if (CUE_SUCCESS != CU_initialize_registry())
		goto err_out;
	/* Add a suite to the registry. */
	psuite = CU_add_suite("Thread Poll", init_suite, clean_suite);
	if (NULL == psuite)
		goto err_out;

	/* Add the tests to the suite. */
	if (NULL == CU_add_test(psuite, "test of test_tp_init1() - threads count = 1", test_tp_init1) ||
	    NULL == CU_add_test(psuite, "test of tp_threads_create()", test_tp_threads_create) ||
	    NULL == CU_add_test(psuite, "test of tp_udata_get()", test_tp_udata_get) ||
	    NULL == CU_add_test(psuite, "test of tp_udata_set()", test_tp_udata_set) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_count_max_get()", test_tp_thread_count_max_get) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_count_get()", test_tp_thread_count_get) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_is_tp_thr()", test_tp_thread_is_tp_thr) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_get()", test_tp_thread_get) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_get_rr()", test_tp_thread_get_rr) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_get_pvt()", test_tp_thread_get_pvt) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_current()", test_tpt_get_current) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_cpu_id()", test_tpt_get_cpu_id) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_tp()", test_tpt_get_tp) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_msg_queue()", test_tpt_get_msg_queue) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_send()", test_tpt_msg_send) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_bsend_ex(0)", test_tpt_msg_bsend_ex1) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_bsend_ex(TP_BMSG_F_SYNC)", test_tpt_msg_bsend_ex2) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_bsend_ex((TP_BMSG_F_SYNC | TP_BMSG_F_SYNC_USLEEP))", test_tpt_msg_bsend_ex3) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_cbsend(0)", test_tpt_msg_cbsend1) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_cbsend(TP_CBMSG_F_ONE_BY_ONE)", test_tpt_msg_cbsend2) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, 0)", test_tpt_ev_add_ex_rd_0) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, TP_F_ONESHOT)", test_tpt_ev_add_ex_rd_oneshot) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, TP_F_DISPATCH)", test_tpt_ev_add_ex_rd_dispatch) ||
#ifdef TP_F_EDGE
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, TP_F_EDGE)", test_tpt_ev_add_ex_rd_edge) ||
#endif
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, 0)", test_tpt_ev_add_ex_rw_0) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, TP_F_ONESHOT)", test_tpt_ev_add_ex_rw_oneshot) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, TP_F_DISPATCH)", test_tpt_ev_add_ex_rw_dispatch) ||
#ifdef TP_F_EDGE
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, TP_F_EDGE)", test_tpt_ev_add_ex_rw_edge) ||
#endif
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, 0)", test_tpt_ev_add_ex_tmr_0) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, TP_F_ONESHOT)", test_tpt_ev_add_ex_tmr_oneshot) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, TP_F_DISPATCH)", test_tpt_ev_add_ex_tmr_dispatch) ||
#ifdef TP_F_EDGE
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, TP_F_EDGE)", test_tpt_ev_add_ex_tmr_edge) ||
#endif
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_PROC, 0)", test_tpt_ev_add_ex_proc_0) ||
	    0 ||
	    NULL == CU_add_test(psuite, "test of test_tp_pvt_msg_flood()", test_tp_pvt_msg_flood) ||
	    NULL == CU_add_test(psuite, "test of test_tp_destroy()", test_tp_destroy) ||
	    NULL == CU_add_test(psuite, "test of test_tp_tpt_hooks()", test_tp_tpt_hooks) ||
	    0 ||
	    NULL == CU_add_test(psuite, "test of test_tp_init16() - threads count = 16", test_tp_init16) ||
	    NULL == CU_add_test(psuite, "test of tp_threads_create()", test_tp_threads_create) ||
	    NULL == CU_add_test(psuite, "test of tp_udata_get()", test_tp_udata_get) ||
	    NULL == CU_add_test(psuite, "test of tp_udata_set()", test_tp_udata_set) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_count_max_get()", test_tp_thread_count_max_get) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_count_get()", test_tp_thread_count_get) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_is_tp_thr()", test_tp_thread_is_tp_thr) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_get()", test_tp_thread_get) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_get_rr()", test_tp_thread_get_rr) ||
	    NULL == CU_add_test(psuite, "test of tp_thread_get_pvt()", test_tp_thread_get_pvt) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_current()", test_tpt_get_current) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_cpu_id()", test_tpt_get_cpu_id) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_tp()", test_tpt_get_tp) ||
	    NULL == CU_add_test(psuite, "test of tpt_get_msg_queue()", test_tpt_get_msg_queue) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_send()", test_tpt_msg_send) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_bsend_ex(0)", test_tpt_msg_bsend_ex1) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_bsend_ex(TP_BMSG_F_SYNC)", test_tpt_msg_bsend_ex2) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_bsend_ex((TP_BMSG_F_SYNC | TP_BMSG_F_SYNC_USLEEP))", test_tpt_msg_bsend_ex3) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_cbsend(0)", test_tpt_msg_cbsend1) ||
	    NULL == CU_add_test(psuite, "test of tpt_msg_cbsend(TP_CBMSG_F_ONE_BY_ONE)", test_tpt_msg_cbsend2) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, 0)", test_tpt_ev_add_ex_rd_0) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, TP_F_ONESHOT)", test_tpt_ev_add_ex_rd_oneshot) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, TP_F_DISPATCH)", test_tpt_ev_add_ex_rd_dispatch) ||
#ifdef TP_F_EDGE
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_READ, TP_F_EDGE)", test_tpt_ev_add_ex_rd_edge) ||
#endif
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, 0)", test_tpt_ev_add_ex_rw_0) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, TP_F_ONESHOT)", test_tpt_ev_add_ex_rw_oneshot) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, TP_F_DISPATCH)", test_tpt_ev_add_ex_rw_dispatch) ||
#ifdef TP_F_EDGE
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_WRITE, TP_F_EDGE)", test_tpt_ev_add_ex_rw_edge) ||
#endif
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, 0)", test_tpt_ev_add_ex_tmr_0) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, TP_F_ONESHOT)", test_tpt_ev_add_ex_tmr_oneshot) ||
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, TP_F_DISPATCH)", test_tpt_ev_add_ex_tmr_dispatch) ||
#ifdef TP_F_EDGE
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_TIMER, TP_F_EDGE)", test_tpt_ev_add_ex_tmr_edge) ||
#endif
	    NULL == CU_add_test(psuite, "test of tpt_ev_add_args(TP_EV_PROC, 0)", test_tpt_ev_add_ex_proc_0) ||
	    0 ||
	    NULL == CU_add_test(psuite, "test of test_tp_pvt_msg_flood()", test_tp_pvt_msg_flood) ||
	    NULL == CU_add_test(psuite, "test of test_tp_destroy()", test_tp_destroy) ||
	    NULL == CU_add_test(psuite, "test of test_tp_tpt_hooks()", test_tp_tpt_hooks) ||
	    0) {
		goto err_out;
	}

	/* Run all tests using the basic interface. */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	printf("\n");
	CU_basic_show_failures(CU_get_failure_list());
	printf("\n\n");
	error = (int)CU_get_number_of_tests_failed();

	/* Run all tests using the automated interface. */
	//CU_automated_run_tests();
	//CU_list_tests_to_file();
	//error = CU_get_number_of_tests_failed();

err_out:
	/* Clean up registry and return. */
	CU_cleanup_registry();
	closelog();

	return (error);
}


static void
test_sleep(const uint64_t msec) { 
	struct timespec rqts;

	/* 1 sec = 1000000000 nanoseconds. */
	rqts.tv_sec = (msec / 1000ul);
	rqts.tv_nsec = ((msec % 1000ul) * 1000000ul);

	for (; 0 != nanosleep(&rqts, &rqts);) {
		if (EINTR != errno)
			break;
	}
}


static int
init_suite(void) {
	int error;

	error = tp_init();
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
tpt_hook_start_cb(tpt_p tpt) {
	size_t tpt_num = tpt_get_num(tpt);

	thr_tls_arr[tpt_num] |= 1;
	CU_ASSERT(0 == tpt_tls_set(tpt, 0, (void*)tpt_num))
	CU_ASSERT(0 == tpt_tls_set(tpt, 1, tpt))
}

static void
tpt_hook_stop_cb(tpt_p tpt) {
	size_t tpt_num = tpt_get_num(tpt);

	thr_tls_arr[tpt_num] |= 2;
	CU_ASSERT(tpt_num == tpt_tls_get_sz(tpt, 0))
	CU_ASSERT(tpt == tpt_tls_get(tpt, 1))
}

static void
test_tp_init(const size_t thr_cnt) {
	int error;
	tp_settings_t s;

	memset(&thr_tls_arr, 0x00, sizeof(thr_tls_arr));

	tp_settings_def(&s);
	threads_count = thr_cnt;
	s.threads_max = thr_cnt;
	s.flags = (TP_S_F_BIND2CPU);
	s.tpt_on_start = tpt_hook_start_cb;
	s.tpt_on_stop = tpt_hook_stop_cb;
	s.udata = &thr_arr;
	error = tp_create(&s, &tp);
	CU_ASSERT(0 == error)
	if (0 != error)
		return;
	/* Wait for all threads start. */
	test_sleep(1500);
}

static void
test_tp_init1(void) {
	test_tp_init(1);
}

static void
test_tp_init16(void) {
	test_tp_init(THREADS_COUNT_MAX);
}

static void
test_tp_destroy(void) {

	tp_shutdown(tp);
	CU_ASSERT(0 == tp_shutdown_wait(tp))
	CU_ASSERT(0 == tp_destroy(tp))
	tp = NULL;
	CU_PASS("tp_destroy()")
}

static void
test_tp_tpt_hooks(void) {

	for (size_t i = 0; i <= threads_count; i ++) { /* Include PVT. */
		CU_ASSERT(3 == thr_tls_arr[i])
	}
	CU_PASS("test_tp_tpt_hooks()")
}

static void
test_tp_udata_get(void) {

	CU_ASSERT(&thr_arr == tp_udata_get(tp))
}

static void
test_tp_udata_set(void) {

	CU_ASSERT(0 == tp_udata_set(tp, NULL))
	CU_ASSERT(NULL == tp_udata_get(tp))
}

static void
test_tp_threads_create(void) {

	CU_ASSERT(0 == tp_threads_create(tp, 0))
	/* Wait for all threads start. */
	test_sleep(1500);
}

static void
test_tp_thread_count_max_get(void) {

	CU_ASSERT(threads_count == tp_thread_count_max_get(tp))
}

static void
test_tp_thread_count_get(void) {

	CU_ASSERT(threads_count == tp_thread_count_get(tp))
}

static void
test_tp_thread_is_tp_thr(void) {

	CU_ASSERT(0 == tp_thread_is_tp_thr(NULL, NULL))
	CU_ASSERT(0 == tp_thread_is_tp_thr(tp, NULL))
	CU_ASSERT(0 != tp_thread_is_tp_thr(tp, tp_thread_get(tp, 0)))
	CU_PASS("tp_thread_is_tp_thr()")
}

static void
test_tp_thread_get(void) {
	size_t i;

	for (i = 0; i < threads_count; i ++) {
		if (i != tpt_get_num(tp_thread_get(tp, i))) {
			CU_FAIL("tpt_get_num()")
			return; /* Fail. */
		}
	}
	CU_PASS("tpt_get_num()")
}

static void
test_tp_thread_get_rr(void) {

	CU_ASSERT(NULL != tp_thread_get_rr(tp))
}

static void
test_tp_thread_get_pvt(void) {

	CU_ASSERT(NULL != tp_thread_get_pvt(tp))
}

static void
test_tpt_get_current(void) {

	CU_ASSERT(NULL == tpt_get_current())
}

static void
test_tpt_get_cpu_id(void) {

	CU_ASSERT(0 == tpt_get_cpu_id(tp_thread_get(tp, 0)))
}

static void
test_tpt_get_tp(void) {

	CU_ASSERT(tp == tpt_get_tp(tp_thread_get(tp, 0)))
}

static void
test_tpt_get_msg_queue(void) {

	CU_ASSERT(NULL != tpt_get_msg_queue(tp_thread_get(tp, 0)))
}



static void
msg_send_cb(tpt_p tpt, void *udata) {

	CU_ASSERT((size_t)udata == tpt_get_num(tpt))

	if ((size_t)udata == tpt_get_num(tpt)) {
		thr_arr[(size_t)udata] = (((size_t)udata) & 0xff);
	}
}
static void
test_tpt_msg_send(void) {
	size_t i;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	for (i = 0; i < threads_count; i ++) {
		if (0 != tpt_msg_send(tp_thread_get(tp, i), NULL,
		    0, msg_send_cb, (void*)i)) {
			CU_FAIL("tpt_msg_send()")
			return; /* Fail. */
		}
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_MS);
	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("tpt_msg_send() - not work.")
			return; /* Fail. */
		}
	}
	CU_PASS("tpt_msg_send()")
}

static void
msg_bsend_cb(tpt_p tpt, void *udata) {

	CU_ASSERT(udata == (void*)tpt_get_tp(tpt))

	if (udata == (void*)tpt_get_tp(tpt)) {
		thr_arr[tpt_get_num(tpt)] = (uint8_t)tpt_get_num(tpt);
	} else {
		thr_arr[tpt_get_num(tpt)] = 0xff;
	}
}
static void
test_tpt_msg_bsend_ex1(void) {
	size_t i;
	size_t send_msg_cnt, error_cnt;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != tpt_msg_bsend_ex(tp, NULL, 0, msg_bsend_cb,
	    (void*)tp, &send_msg_cnt, &error_cnt)) {
		CU_FAIL("tpt_msg_bsend_ex()")
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_MS);

	if (threads_count != send_msg_cnt ||
	    0 != error_cnt) {
		CU_FAIL("tpt_msg_bsend_ex() - not all received.")
		return; /* Fail. */
	}

	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("tpt_msg_bsend_ex() - not work.")
			return; /* Fail. */
		}
	}
	CU_PASS("tpt_msg_bsend_ex()")
}
static void
test_tpt_msg_bsend_ex2(void) {
	size_t i;
	size_t send_msg_cnt, error_cnt;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != tpt_msg_bsend_ex(tp, NULL, TP_BMSG_F_SYNC, msg_bsend_cb,
	    (void*)tp, &send_msg_cnt, &error_cnt)) {
		CU_FAIL("tpt_msg_bsend_ex(TP_BMSG_F_SYNC)")
		return; /* Fail. */
	}

	if (threads_count != send_msg_cnt ||
	    0 != error_cnt) {
		CU_FAIL("tpt_msg_bsend_ex(TP_BMSG_F_SYNC) - not all received.")
		return; /* Fail. */
	}

	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("tpt_msg_bsend_ex(TP_BMSG_F_SYNC) - not work.")
			return; /* Fail. */
		}
	}
	CU_PASS("tpt_msg_bsend_ex(TP_BMSG_F_SYNC)")
}
static void
test_tpt_msg_bsend_ex3(void) {
	size_t i;
	size_t send_msg_cnt, error_cnt;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != tpt_msg_bsend_ex(tp, NULL,
	    (TP_BMSG_F_SYNC | TP_BMSG_F_SYNC_USLEEP), msg_bsend_cb,
	    (void*)tp, &send_msg_cnt, &error_cnt)) {
		CU_FAIL("tpt_msg_bsend_ex((TP_BMSG_F_SYNC | TP_BMSG_F_SYNC_USLEEP))")
		return; /* Fail. */
	}

	if (threads_count != send_msg_cnt ||
	    0 != error_cnt) {
		CU_FAIL("tpt_msg_bsend_ex((TP_BMSG_F_SYNC | TP_BMSG_F_SYNC_USLEEP)) - not all received.")
		return; /* Fail. */
	}

	for (i = 0; i < threads_count; i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("tpt_msg_bsend_ex((TP_BMSG_F_SYNC | TP_BMSG_F_SYNC_USLEEP)) - not work.")
			return; /* Fail. */
		}
	}
	CU_PASS("tpt_msg_bsend_ex((TP_BMSG_F_SYNC | TP_BMSG_F_SYNC_USLEEP))")
}

static void
msg_cbsend_cb(tpt_p tpt, void *udata) {

	CU_ASSERT(udata == (void*)tpt_get_tp(tpt))

	if (udata == (void*)tpt_get_tp(tpt)) {
		thr_arr[tpt_get_num(tpt)] = (uint8_t)tpt_get_num(tpt);
	}
}
static void
msg_cbsend_done_cb(tpt_p tpt, size_t send_msg_cnt,
    size_t error_cnt, void *udata) {

	CU_ASSERT(udata == (void*)tpt_get_tp(tpt))
	CU_ASSERT(threads_count == send_msg_cnt)
	CU_ASSERT(0 == error_cnt)

	if (udata == (void*)tpt_get_tp(tpt) &&
	    threads_count == send_msg_cnt &&
	    0 == error_cnt) {
		thr_arr[threads_count] = (uint8_t)threads_count;
	}
}
static void
test_tpt_msg_cbsend1(void) {
	size_t i;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != tpt_msg_cbsend(tp, tp_thread_get(tp, 0),
	    0, msg_cbsend_cb, (void*)tp, msg_cbsend_done_cb)) {
		CU_FAIL("tpt_msg_cbsend()")
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_MS);
	for (i = 0; i < (threads_count + 1); i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("tpt_msg_cbsend() - not work.")
			return; /* Fail. */
		}
	}
	CU_PASS("tpt_msg_cbsend()")
}
static void
test_tpt_msg_cbsend2(void) {
	size_t i;

	memset(thr_arr, 0xff, sizeof(thr_arr));

	if (0 != tpt_msg_cbsend(tp, tp_thread_get(tp, 0),
	    TP_CBMSG_F_ONE_BY_ONE, msg_cbsend_cb, (void*)tp,
	    msg_cbsend_done_cb)) {
		CU_FAIL("tpt_msg_cbsend(TP_CBMSG_F_ONE_BY_ONE)")
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_MS);
	for (i = 0; i < (threads_count + 1); i ++) {
		if (i != thr_arr[i]) {
			CU_FAIL("tpt_msg_cbsend(TP_CBMSG_F_ONE_BY_ONE) - not work.")
			return; /* Fail. */
		}
	}
	CU_PASS("tpt_msg_cbsend(TP_CBMSG_F_ONE_BY_ONE)")
}


static void
tpt_ev_add_r_cb(tp_event_p ev, tp_udata_p tp_udata) {

	CU_ASSERT(0 != ev->data)
	CU_ASSERT(TP_EV_READ == ev->event)
	CU_ASSERT(tpt_ev_add_r_cb == tp_udata->cb_func)
	CU_ASSERT(pipe_fd[0] == (int)tp_udata->ident)

	//read(pipe_fd[0], buf, sizeof(buf));
	if (0 != ev->data &&
	    TP_EV_READ == ev->event &&
	    tpt_ev_add_r_cb == tp_udata->cb_func &&
	    pipe_fd[0] == (int)tp_udata->ident) {
		thr_arr[0] ++;
		if (TEST_EV_CNT_MAX <= thr_arr[0]) {
			tpt_ev_enable_args1(0, TP_EV_READ, tp_udata);
		}
	}
}
static void
test_tpt_ev_add_ex_rd(uint16_t flags, uint8_t res, int remove_ok) {
	tp_udata_t tp_udata;
	uint8_t buf[(TEST_EV_CNT_MAX * 2)];

	/* Init. */
	thr_arr[0] = 0;
	memset(&tp_udata, 0x00, sizeof(tp_udata));
	read(pipe_fd[0], buf, sizeof(buf));

	tp_udata.cb_func = tpt_ev_add_r_cb;
	tp_udata.ident = (uintptr_t)pipe_fd[0];
	if (0 != tpt_ev_add_args(tp_thread_get(tp, 0), TP_EV_READ,
	    flags, 0, 0, &tp_udata)) {
		CU_FAIL("tpt_ev_add_args(TP_EV_READ)") /* Fail. */
		read(pipe_fd[0], buf, sizeof(buf));
		tpt_ev_del_args1(TP_EV_READ, &tp_udata);
		return; /* Fail. */
	}
	CU_ASSERT(1 == write(pipe_fd[1], "1", 1))
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_MS);
	if (res != thr_arr[0]) {
		CU_FAIL("tpt_ev_add_args(TP_EV_READ) - not work") /* Fail. */
		LOG_CONS_INFO_FMT("%i", (int)thr_arr[0]);
	}
	/* Clean. */
	read(pipe_fd[0], buf, sizeof(buf));
	if (0 != remove_ok) {
		CU_ASSERT(0 == tpt_ev_del_args1(TP_EV_READ, &tp_udata))
	}
	CU_ASSERT(0 != tpt_ev_del_args1(TP_EV_READ, &tp_udata))
}
static void
test_tpt_ev_add_ex_rd_0(void) {

	test_tpt_ev_add_ex_rd(0, TEST_EV_CNT_MAX, 1);
}
static void
test_tpt_ev_add_ex_rd_oneshot(void) {

	test_tpt_ev_add_ex_rd(TP_F_ONESHOT, 1, 0);
}
static void
test_tpt_ev_add_ex_rd_dispatch(void) {

	test_tpt_ev_add_ex_rd(TP_F_DISPATCH, 1, 1);
}
#ifdef TP_F_EDGE
static void
test_tpt_ev_add_ex_rd_edge(void) {

	test_tpt_ev_add_ex_rd(TP_F_EDGE, 1, 1);
}
#endif


static void
tpt_ev_add_w_cb(tp_event_p ev, tp_udata_p tp_udata) {

	CU_ASSERT(0 != ev->data)
	CU_ASSERT(TP_EV_WRITE == ev->event)
	CU_ASSERT(tpt_ev_add_w_cb == tp_udata->cb_func)
	CU_ASSERT(pipe_fd[1] == (int)tp_udata->ident)

	if (0 != ev->data &&
	    TP_EV_WRITE == ev->event &&
	    tpt_ev_add_w_cb == tp_udata->cb_func &&
	    pipe_fd[1] == (int)tp_udata->ident &&
	    1 == write(pipe_fd[1], "1", 1) &&
	    1 == write(pipe_fd[1], "1", 1)) { /* Dup for TP_F_ONESHOT test. */
		thr_arr[0] ++;
		if (TEST_EV_CNT_MAX <= thr_arr[0]) {
			tpt_ev_enable_args1(0, TP_EV_WRITE, tp_udata);
		}
	}
}
static void
test_tpt_ev_add_ex_rw(uint16_t flags, uint8_t res, int remove_ok) {
	tp_udata_t tp_udata;
	uint8_t buf[(TEST_EV_CNT_MAX * 2)];

	/* Init. */
	thr_arr[0] = 0;
	memset(&tp_udata, 0x00, sizeof(tp_udata));
	read(pipe_fd[0], buf, sizeof(buf));

	tp_udata.cb_func = tpt_ev_add_w_cb;
	tp_udata.ident = (uintptr_t)pipe_fd[1];
	if (0 != tpt_ev_add_args(tp_thread_get(tp, 0), TP_EV_WRITE,
	    flags, 0, 0, &tp_udata)) {
		CU_FAIL("tpt_ev_add_args(TP_EV_WRITE)") /* Fail. */
		read(pipe_fd[0], buf, sizeof(buf));
		tpt_ev_del_args1(TP_EV_WRITE, &tp_udata);
		return; /* Fail. */
	}
	CU_ASSERT(1 == write(pipe_fd[1], "1", 1))
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_MS);
	if (res != thr_arr[0]) {
		CU_FAIL("tpt_ev_add_args(TP_EV_WRITE) - not work") /* Fail. */
		LOG_CONS_INFO_FMT("%i", (int)thr_arr[0]);
	}
	/* Clean. */
	read(pipe_fd[0], buf, sizeof(buf));
	if (0 != remove_ok) {
		CU_ASSERT(0 == tpt_ev_del_args1(TP_EV_WRITE, &tp_udata))
	}
	CU_ASSERT(0 != tpt_ev_del_args1(TP_EV_WRITE, &tp_udata))
}
static void
test_tpt_ev_add_ex_rw_0(void) {

	test_tpt_ev_add_ex_rw(0, TEST_EV_CNT_MAX, 1);
}
static void
test_tpt_ev_add_ex_rw_oneshot(void) {

	test_tpt_ev_add_ex_rw(TP_F_ONESHOT, 1, 0);
}
static void
test_tpt_ev_add_ex_rw_dispatch(void) {

	test_tpt_ev_add_ex_rw(TP_F_DISPATCH, 1, 1);
}
#ifdef TP_F_EDGE
static void
test_tpt_ev_add_ex_rw_edge(void) {

	test_tpt_ev_add_ex_rw(TP_F_EDGE, 1, 1);
}
#endif


static void
tpt_ev_add_tmr_cb(tp_event_p ev, tp_udata_p tp_udata) {

	CU_ASSERT(TP_EV_TIMER == ev->event)
	CU_ASSERT(tpt_ev_add_tmr_cb == tp_udata->cb_func)
	CU_ASSERT(TEST_TIMER_ID == tp_udata->ident)

	if (TP_EV_TIMER == ev->event &&
	    tpt_ev_add_tmr_cb == tp_udata->cb_func &&
	    TEST_TIMER_ID == tp_udata->ident) {
		thr_arr[0] ++;
		if (TEST_EV_CNT_MAX <= thr_arr[0]) {
			tpt_ev_enable_args1(0, TP_EV_TIMER, tp_udata);
		}
	}
}
static void
test_tpt_ev_add_ex_tmr(uint16_t flags, uint8_t res, int remove_ok) {
	tp_udata_t tp_udata;

	/* Init. */
	thr_arr[0] = 0;
	memset(&tp_udata, 0x00, sizeof(tp_udata));

	tp_udata.cb_func = tpt_ev_add_tmr_cb;
	tp_udata.ident = TEST_TIMER_ID;
	if (0 != tpt_ev_add_args(tp_thread_get(tp, 0), TP_EV_TIMER,
	    flags, TP_FF_T_MSEC, TEST_TIMER_INTERVAL, &tp_udata)) {
		CU_FAIL("tpt_ev_add_args(TP_EV_TIMER)") /* Fail. */
		tpt_ev_del_args1(TP_EV_TIMER, &tp_udata);
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep((TEST_SLEEP_TIME_MS + (res * TEST_TIMER_INTERVAL * 2)));
	if (res != thr_arr[0]) {
		CU_FAIL("tpt_ev_add_args(TP_EV_TIMER) - not work") /* Fail. */
		LOG_CONS_INFO_FMT("%i", (int)thr_arr[0]);
	}
	/* Clean. */
	if (0 != remove_ok) {
		CU_ASSERT(0 == tpt_ev_del_args1(TP_EV_TIMER, &tp_udata))
	}
	CU_ASSERT(0 != tpt_ev_del_args1(TP_EV_TIMER, &tp_udata))
}
static void
test_tpt_ev_add_ex_tmr_0(void) {

	test_tpt_ev_add_ex_tmr(0, TEST_EV_CNT_MAX, 1);
}
static void
test_tpt_ev_add_ex_tmr_oneshot(void) {

	test_tpt_ev_add_ex_tmr(TP_F_ONESHOT, 1, 0);
}
static void
test_tpt_ev_add_ex_tmr_dispatch(void) {

	test_tpt_ev_add_ex_tmr(TP_F_DISPATCH, 1, 1);
}
#ifdef TP_F_EDGE
static void
test_tpt_ev_add_ex_tmr_edge(void) {

	test_tpt_ev_add_ex_tmr(TP_F_EDGE, TEST_EV_CNT_MAX, 1);
}
#endif


static void
tpt_ev_add_proc_cb(tp_event_p ev, tp_udata_p tp_udata) {

	CU_ASSERT(TP_EV_PROC == ev->event)
	CU_ASSERT(tpt_ev_add_proc_cb == tp_udata->cb_func)
	CU_ASSERT((uintptr_t)pid == tp_udata->ident)

	if (TP_EV_PROC == ev->event &&
	    tpt_ev_add_proc_cb == tp_udata->cb_func &&
	    (uintptr_t)pid == tp_udata->ident) {
		thr_arr[0] ++;
		if (TEST_EV_CNT_MAX <= thr_arr[0]) {
			tpt_ev_enable_args1(0, TP_EV_PROC, tp_udata);
		}
	}
}
static void
test_tpt_ev_add_ex_proc_0(void) {
	tp_udata_t tp_udata;
	posix_spawn_file_actions_t actions;
	char *const argv[] = { "sleep", TEST_PROC_INTERVAL_STR, NULL, NULL };

	/* Init. */
	thr_arr[0] = 0;
	memset(&tp_udata, 0x00, sizeof(tp_udata));

	/* Start process to track... */
	CU_ASSERT(0 == posix_spawn_file_actions_init(&actions))
	/* Do not pass all fd~s to child. */
#ifdef HAVE_POSIX_SPAWN_FILE_ACTIONS_ADDCLOSEFROM_NP
	CU_ASSERT(0 == posix_spawn_file_actions_addclosefrom_np(&actions, (STDERR_FILENO + 1)))
#elif defined(CLOSE_RANGE_CLOEXEC)
	CU_ASSERT(-1 != close_range((STDERR_FILENO + 1), ~0U, CLOSE_RANGE_CLOEXEC))
#else
	for (int tfd = (STDERR_FILENO + 1); tfd < 1024; tfd ++) {
		fcntl(tfd, F_SETFD, FD_CLOEXEC);
	}
#endif
	CU_ASSERT(0 == posix_spawnp(&pid, argv[0], &actions, NULL, argv, environ))
	posix_spawn_file_actions_destroy(&actions);


	tp_udata.cb_func = tpt_ev_add_proc_cb;
	tp_udata.ident = (uintptr_t)pid;
	if (0 != tpt_ev_add_args(tp_thread_get(tp, 0), TP_EV_PROC, 0,
	    TP_FF_P_EXIT, 0, &tp_udata)) {
		CU_FAIL("tpt_ev_add_args(TP_EV_PROC)") /* Fail. */
		tpt_ev_del_args1(TP_EV_PROC, &tp_udata);
		return; /* Fail. */
	}
	/* Wait for all threads process. */
	test_sleep((TEST_SLEEP_TIME_MS + ((TEST_PROC_INTERVAL * 1000) * 2)));
	if (1 != thr_arr[0]) {
		CU_FAIL("tpt_ev_add_args(TP_EV_PROC) - not work") /* Fail. */
		LOG_CONS_INFO_FMT("%i", (int)thr_arr[0]);
	}
	/* Clean. */
	CU_ASSERT(0 != tpt_ev_del_args1(TP_EV_PROC, &tp_udata))
}


static void
msg_send_pvt_msg_flood_cb(tpt_p tpt __unused, void *udata) {
	size_t tpt_num = tpt_get_num(tpt_get_current()); /* Get real thread. */

	CU_ASSERT(NULL == udata)

	thr_flood_arr[tpt_num] ++;
}
static void
test_tp_pvt_msg_flood(void) {
	int error;
	tpt_p tpt_pvt = tp_thread_get_pvt(tp);
	size_t i, fired_cnt = 0;
	const size_t fired_cnt_target = (threads_count * 16384);

	CU_ASSERT(NULL != tpt_pvt)
	memset(&thr_flood_arr, 0x00, sizeof(thr_flood_arr));
	for (i = 0; i < fired_cnt_target; i ++) {
		for (;;) {
			error = tpt_msg_send(tpt_pvt, NULL,
			    0, msg_send_pvt_msg_flood_cb, NULL);
			if (EAGAIN != error)
				break;
			test_sleep(1);
		}
		if (0 != error) {
			CU_FAIL("tpt_msg_send()")
			return; /* Fail. */
		}
	}
	/* Wait for all threads process. */
	test_sleep(TEST_SLEEP_TIME_MS * 2);
	for (i = 0; i < threads_count; i ++) {
		fired_cnt += thr_flood_arr[i];
	}
	CU_ASSERT(fired_cnt == fired_cnt_target)
	CU_PASS("test_tp_pvt_msg_flood()")
}
