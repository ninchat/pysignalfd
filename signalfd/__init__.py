__all__ = [
	"enqueue",
	"init",
	"iterate",
	"mask",
	"read",
	"reset",
]

import errno
import select
import signal

from _signalfd import *

def identity(signum):
	return signum

def iterate():
	fd = init()

	try:
		while True:
			try:
				r, _, _ = select.select([fd], [], [])
			except select.error as e:
				if e.args[0] == errno.EINTR:
					continue
				raise

			if r:
				signum = read()
				if signum > 0:
					yield signum
	finally:
		reset()

def enqueue(queue, wrapper=identity, terminator=StopIteration):
	try:
		for signum in iterate():
			queue.put(wrapper(signum))
	finally:
		queue.put(terminator)
