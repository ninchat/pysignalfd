/*
 * Copyright (c) 2010  Sulake Corporation Oy
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Python.h>

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

static int pysignalfd_read_fd = -1;
static int pysignalfd_write_fd = -1;

static void pysignalfd_handler(int num)
{
	char buf = num;

	if (write(pysignalfd_write_fd, &buf, 1) < 0)
		perror("write");
}

static void pysignalfd_parent(int fd[2], pid_t pid)
{
	int num;

	close(fd[0]);
	pysignalfd_write_fd = fd[1];

	for (num = 1; num < 32; num++)
		switch (num) {
		case SIGABRT:
		case SIGBUS:
		case SIGCHLD:
		case SIGILL:
		case SIGKILL:
		case SIGSEGV:
		case SIGSTOP:
			break;

		case SIGPIPE:
			if (signal(num, SIG_IGN) == SIG_ERR)
				perror("signal");
			break;

		default:
			if (signal(num, pysignalfd_handler) == SIG_ERR)
				perror("signal");
			break;
		}

	while (true) {
		int status;

		if (waitpid(pid, &status, 0) < 0) {
			if (errno == EINTR)
				continue;

			abort();
		}

		_exit(status);
	}
}

static int pysignalfd_init(void)
{
	int fd[2];
	pid_t pid;
	sigset_t set;

	if (pipe(fd) < 0) {
		perror("pipe");
		goto no_pipe;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		goto no_fork;
	}

	if (pid > 0)
		pysignalfd_parent(fd, pid);

	if (sigfillset(&set) < 0) {
		perror("sigfillset");
		goto no_fill;
	}

	if (sigprocmask(SIG_SETMASK, &set, NULL) < 0) {
		perror("sigprocmask");
		goto no_mask;
	}

	pysignalfd_read_fd = fd[0];
	close(fd[1]);

	return 0;

no_mask:
no_fill:
no_fork:
	close(fd[0]);
	close(fd[1]);
no_pipe:
	return -1;
}

static PyObject *pysignalfd_get_fd(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(pysignalfd_read_fd);
}

static PyMethodDef pysignalfd_methods[] = {
	{ "get_fd", pysignalfd_get_fd, METH_NOARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC initsignalfd(void)
{
	if (pysignalfd_init() < 0)
		return;

	Py_InitModule("signalfd", pysignalfd_methods);
}
