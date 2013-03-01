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

#include <signal.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>

static int pysignalfd_fd = -1;

static void pysignalfd_close(void)
{
	if (pysignalfd_fd >= 0) {
		close(pysignalfd_fd);
		pysignalfd_fd = -1;
	}
}

static int pysignalfd_fill_and_mask(sigset_t *set)
{
	if (sigfillset(set) < 0)
		return -1;

	if (sigdelset(set, SIGABRT) < 0 ||
	    sigdelset(set, SIGBUS) < 0 ||
	    sigdelset(set, SIGFPE) < 0 ||
	    sigdelset(set, SIGILL) < 0 ||
	    sigdelset(set, SIGKILL) < 0 ||
	    sigdelset(set, SIGSEGV) < 0 ||
	    sigdelset(set, SIGSTOP) < 0)
		return -1;

	if (pthread_sigmask(SIG_SETMASK, set, NULL) < 0)
		return -1;

	return 0;
}

static PyObject *pysignalfd_mask(PyObject *self, PyObject *args)
{
	sigset_t set;

	if (pysignalfd_fill_and_mask(&set) < 0)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *pysignalfd_init(PyObject *self, PyObject *args)
{
	sigset_t set;

	pysignalfd_close();

	if (pysignalfd_fill_and_mask(&set) < 0)
		return NULL;

	pysignalfd_fd = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);
	if (pysignalfd_fd < 0)
		return NULL;

	return PyLong_FromLong(pysignalfd_fd);
}

static PyObject *pysignalfd_read(PyObject *self, PyObject *args)
{
	struct signalfd_siginfo buf;
	int signo = 0;
	ssize_t len;

	if (pysignalfd_fd < 0)
		return NULL;

	len = read(pysignalfd_fd, &buf, sizeof (buf));

	if (len < 0 && (errno != EAGAIN && errno != EINTR))
		return NULL;

	if (len >= 0 && (size_t) len >= sizeof (buf))
		signo = buf.ssi_signo;

	return PyLong_FromLong(signo);
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
	{ "read", pysignalfd_read, METH_NOARGS, NULL },
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
