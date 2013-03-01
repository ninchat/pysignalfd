/*
 * Copyright (c) 2010, 2013  Somia Reality Oy
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Python.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <signal.h>
#include <stddef.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

static pid_t pysignalfd_pid = 0;
static int pysignalfd_pipe[2] = { -1, -1 };

static void pysignalfd_handler(int signum)
{
	int saved_errno = errno;
	char buf = signum;

	while (getpid() == pysignalfd_pid &&
	       write(pysignalfd_pipe[1], &buf, 1) < 0 &&
	       errno == EINTR) {
	}

	errno = saved_errno;
}

static void pysignalfd_close(void)
{
	int i;

	pysignalfd_pid = 0;

	for (i = 1; i < 32; i++) {
		switch (i) {
		case SIGABRT:
		case SIGBUS:
		case SIGFPE:
		case SIGILL:
		case SIGKILL:
		case SIGSEGV:
		case SIGSTOP:
			break;

		default:
			signal(i, SIG_DFL);
			break;
		}
	}

	for (i = 0; i < 2; i++) {
		if (pysignalfd_pipe[i] >= 0) {
			close(pysignalfd_pipe[i]);
			pysignalfd_pipe[i] = -1;
		}
	}
}

static PyObject *pysignalfd_mask(PyObject *self, PyObject *args)
{
	sigset_t set;

	if (sigfillset(&set) < 0)
		return NULL;

	if (sigdelset(&set, SIGABRT) < 0 ||
	    sigdelset(&set, SIGBUS) < 0 ||
	    sigdelset(&set, SIGFPE) < 0 ||
	    sigdelset(&set, SIGILL) < 0 ||
	    sigdelset(&set, SIGKILL) < 0 ||
	    sigdelset(&set, SIGSEGV) < 0 ||
	    sigdelset(&set, SIGSTOP) < 0)
		return NULL;

	if (pthread_sigmask(SIG_SETMASK, &set, NULL) < 0)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *pysignalfd_init(PyObject *self, PyObject *args)
{
	int i;
	sigset_t set;
	struct sigaction action = {
		.sa_flags = SA_NOCLDSTOP | SA_RESTART,
	};

	pysignalfd_close();

	if (pipe2(pysignalfd_pipe, O_CLOEXEC) < 0)
		return NULL;

	for (i = 1; i < 32; i++)
		switch (i) {
		case SIGABRT:
		case SIGBUS:
		case SIGILL:
		case SIGKILL:
		case SIGSEGV:
		case SIGSTOP:
			break;

		case SIGPIPE:
			action.sa_handler = SIG_IGN;
			sigaction(i, &action, NULL);
			break;

		default:
			action.sa_handler = pysignalfd_handler;
			sigaction(i, &action, NULL);
			break;
		}

	pysignalfd_pid = getpid();

	if (sigemptyset(&set) < 0)
		return NULL;

	if (pthread_sigmask(SIG_SETMASK, &set, NULL) < 0)
		return NULL;

	return PyLong_FromLong(pysignalfd_pipe[0]);
}

static PyObject *pysignalfd_reset(PyObject *self, PyObject *args)
{
	sigset_t set;

	pysignalfd_close();

	if (sigemptyset(&set) < 0)
		return NULL;

	if (pthread_sigmask(SIG_SETMASK, &set, NULL) < 0)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef pysignalfd_methods[] = {
	{ "init", pysignalfd_init, METH_NOARGS, NULL },
	{ "mask", pysignalfd_mask, METH_NOARGS, NULL },
	{ "reset", pysignalfd_reset, METH_NOARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

#if PY_MAJOR_VERSION < 3

PyMODINIT_FUNC init_signalfd(void)
{
	Py_InitModule("_signalfd", pysignalfd_methods);
}

#else

static struct PyModuleDef pysignalfd_module = {
	PyModuleDef_HEAD_INIT,
	"_signalfd",
	NULL,
	-1,
	pysignalfd_methods,
};

PyMODINIT_FUNC PyInit__signalfd(void)
{
	return PyModule_Create(&pysignalfd_module);
}

#endif
