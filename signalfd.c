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

#include <signal.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>

static int pysignalfd_pipe[2] = { -1, -1 };

static void pysignalfd_handler(int signum)
{
	char buf = signum;

	if (write(pysignalfd_pipe[1], &buf, 1) < 0)
		;
}

static PyObject *pysignalfd_init(PyObject *self, PyObject *args)
{
	int i;
	struct sigaction action = {
		.sa_flags = SA_NOCLDSTOP | SA_RESTART,
	};

	if (pipe(pysignalfd_pipe) < 0)
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

	return PyLong_FromLong(pysignalfd_pipe[0]);
}

static PyObject *pysignalfd_reset(PyObject *self, PyObject *args)
{
	int i;

	for (i = 1; i < 32; i++)
		switch (i) {
		case SIGABRT:
		case SIGBUS:
		case SIGILL:
		case SIGKILL:
		case SIGSEGV:
		case SIGSTOP:
			break;

		default:
			signal(i, SIG_DFL);
			break;
		}

	for (i = 0; i < 2; i++)
		if (pysignalfd_pipe[i] >= 0) {
			close(pysignalfd_pipe[i]);
			pysignalfd_pipe[i] = -1;
		}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef pysignalfd_methods[] = {
	{ "init", pysignalfd_init, METH_NOARGS, NULL },
	{ "reset", pysignalfd_reset, METH_NOARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

#if PY_MAJOR_VERSION < 3

PyMODINIT_FUNC initsignalfd(void)
{
	Py_InitModule("signalfd", pysignalfd_methods);
}

#else

static struct PyModuleDef pysignalfd_module = {
	PyModuleDef_HEAD_INIT,
	"signalfd",
	NULL,
	-1,
	pysignalfd_methods,
};

PyMODINIT_FUNC PyInit_signalfd(void)
{
	return PyModule_Create(&pysignalfd_module);
}

#endif
