/*-
 * Copyright (c) 2011 - 2017 Rozhuk Ivan <rozhuk.im@gmail.com>
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
#include <sys/stat.h> /* chmod, fchmod, umask */
#include <sys/uio.h> /* readv, preadv, writev, pwritev */
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h> /* open, fcntl */
#include <stdio.h>  /* snprintf, fprintf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strnlen, strerror... */
#include <unistd.h> /* close, write, sysconf */
#include <stdlib.h> /* malloc, exit */
#include <signal.h>

#include "utils/sys.h"


void
signal_install(sig_t func) {

	signal(SIGINT, func);
	signal(SIGTERM, func);
	//signal(SIGKILL, func);
	signal(SIGHUP, func);
	signal(SIGUSR1, func);
	signal(SIGUSR2, func);
	signal(SIGPIPE, SIG_IGN);
}

void
make_daemon(void) {
	int error;

	switch (fork()) {
	case -1:
		error = errno;
		fprintf(stderr, "make_daemon: fork() failed: %i %s\n",
		    error, strerror(error));
		exit(error);
		/* return; */
	case 0: /* Child. */
		break;
	default: /* Parent. */
		exit(0);
	}

	/* Child... */
	setsid();
	setpgid(getpid(), 0);

	/* Close stdin, stdout, stderr. */
	close(0);
	close(1);
	close(2);
}

int
write_pid(const char *file_name) {
	int fd;
	char data[64];
	size_t data_size;

	if (NULL == file_name)
		return (EINVAL);

	fd = open(file_name, (O_WRONLY | O_CREAT | O_TRUNC), 0644);
	if (-1 == fd)
		return (errno);
	data_size = (size_t)snprintf(data, sizeof(data), "%d", getpid());
	write(fd, data, data_size);
	fchmod(fd, (S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH));
	close(fd);

	return (0);
}

int
set_user_and_group(uid_t pw_uid, gid_t pw_gid) {
	int error = 0;
	struct passwd *pwd, pwd_buf;
	char buffer[4096];

	if (0 == pw_uid || 0 == pw_gid)
		return (EINVAL);

	error = getpwuid_r(pw_uid, &pwd_buf, buffer, sizeof(buffer), &pwd);
	if (0 != error) {
		error = errno;
		fprintf(stderr, "set_user_and_group: getpwuid_r() error %i: %s\n",
		    error, strerror(error));
		return (error);
	}

	if (0 != setgid(pw_gid)) {
		error = errno;
		fprintf(stderr, "set_user_and_group: setgid() error %i: %s\n",
		    error, strerror(error));
	}
	if (0 != initgroups(pwd->pw_name, pw_gid)) {
		error = errno;
		fprintf(stderr, "set_user_and_group: initgroups() error %i: %s\n",
		    error, strerror(error));
	}
	if (0 != setgroups(1, &pwd->pw_gid)) {
		error = errno;
		fprintf(stderr, "set_user_and_group: setgroups() error %i: %s\n",
		    error, strerror(error));
	}
	if (0 != setuid(pw_uid)) {
		error = errno;
		fprintf(stderr, "set_user_and_group: setuid() error %i: %s\n",
		    error, strerror(error));
	}

	return (error);
}

int
user_home_dir_get(char *buf, size_t buf_size, size_t *buf_size_ret) {
	const char *homedir;
	char tmbuf[4096];
	size_t homedir_size;
	struct passwd pwd, *pwdres;

	homedir = getenv("HOME");
	if (NULL == homedir) {
		if (0 == getpwuid_r(getuid(), &pwd, tmbuf, sizeof(tmbuf), &pwdres)) {
			homedir = pwd.pw_dir;
		}
	}
	if (NULL == homedir)
		return (errno);
	homedir_size = strlen(homedir);
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = homedir_size;
	}
	if (NULL == buf && buf_size < homedir_size)
		return (-1);
	memcpy(buf, homedir, homedir_size);

	return (0);
}

int
read_file(const char *file_name, size_t file_name_size, off_t offset,
    size_t size, size_t max_size, uint8_t **buf, size_t *buf_size) {
	int fd, error;
	ssize_t rd;
	char filename[1024];
	struct stat sb;

	if (NULL == file_name || sizeof(filename) <= file_name_size ||
	    NULL == buf || NULL == buf_size)
		return (EINVAL);
	if (0 == file_name_size) {
		file_name_size = strnlen(file_name, (sizeof(filename) - 1));
	}
	memcpy(filename, file_name, file_name_size);
	filename[file_name_size] = 0;

	/* Open file. */
	fd = open(filename, O_RDONLY);
	if (-1 == fd)
		return (errno);
	/* Get file size. */
	if (0 != fstat(fd, &sb)) {
		error = errno;
		goto err_out;
	}
	/* Check size and offset. */
	if (0 != size) {
		if ((offset + (off_t)size) > sb.st_size) {
			error = EINVAL;
			goto err_out;
		}
	} else {
		/* Check overflow. */
		if (offset >= sb.st_size) {
			error = EINVAL;
			goto err_out;
		}
		size = (size_t)(sb.st_size - offset);
		if (0 != max_size && max_size < size) {
			(*buf_size) = size;
			error = EFBIG;
			goto err_out;
		}
	}
	/* Allocate buf for file content. */
	(*buf_size) = size;
	(*buf) = malloc((size + sizeof(void*)));
	if (NULL == (*buf)) {
		error = ENOMEM;
		goto err_out;
	}
	/* Read file content. */
	rd = pread(fd, (*buf), size, offset);
	close(fd);
	if (-1 == rd) {
		error = errno;
		free((*buf));
		(*buf) = NULL;
		return (error);
	}
	(*buf)[size] = 0;

	return (0);

err_out:
	close(fd);

	return (error);
}

int
read_file_buf(const char *file_name, size_t file_name_size, uint8_t *buf,
    size_t buf_size, size_t *buf_size_ret) {
	int fd;
	size_t rd;
	char filename[1024];

	if (NULL == file_name || sizeof(filename) <= file_name_size ||
	    NULL == buf || 0 == buf_size)
		return (EINVAL);

	if (0 == file_name_size) {
		file_name_size = strnlen(file_name, (sizeof(filename) - 1));
	}
	memcpy(filename, file_name, file_name_size);
	filename[file_name_size] = 0;
	/* Open file. */
	fd = open(filename, O_RDONLY);
	if (-1 == fd)
		return (errno);
	/* Read file content. */
	rd = (size_t)read(fd, buf, buf_size);
	close(fd);
	if ((size_t)-1 == rd)
		return (errno);
	if (buf_size > rd) { /* Zeroize end. */
		buf[rd] = 0;
	}
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = rd;
	}

	return (0);
}

int
file_size_get(const char *file_name, size_t file_name_size, off_t *file_size) {
	struct stat sb;
	char filename[1024];

	if (NULL == file_name || sizeof(filename) <= file_name_size ||
	    NULL == file_size)
		return (EINVAL);
	if (0 == file_name_size) {
		file_name_size = strnlen(file_name, (sizeof(filename) - 1));
	}
	memcpy(filename, file_name, file_name_size);
	filename[file_name_size] = 0;
	if (0 != stat(filename, &sb))
		return (errno);
	(*file_size) = sb.st_size;

	return (0);
}

int
get_cpu_count(void) {
	int ret;

	ret = (int)sysconf(_SC_NPROCESSORS_ONLN);
	if (-1 == ret) {
		ret = 1;
	}

	return (ret);
}

/* Set file/socket to non blocking mode */
int
fd_set_nonblocking(uintptr_t fd, int nonblocked) {
	int opts;

	if ((uintptr_t)-1 == fd)
		return (EINVAL);

	opts = fcntl((int)fd, F_GETFL); /* Read current options. */
	if (-1 == opts)
		return (errno);
	if (0 == nonblocked) {
		if (0 == (opts & O_NONBLOCK))
			return (0); /* Allready set. */
		opts &= ~O_NONBLOCK;
	} else {
		if (0 != (opts & O_NONBLOCK))
			return (0); /* Allready set. */
		opts |= O_NONBLOCK;
	}
	if (-1 == fcntl((int)fd, F_SETFL, opts)) /* Update options. */
		return (errno);

	return (0);
}
