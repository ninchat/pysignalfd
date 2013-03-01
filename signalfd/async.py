__all__ = [
	"dispatcher",
]

import asyncore
import signal

import signalfd

class dispatcher(asyncore.file_dispatcher):

	def __init__(self, map=None):
		fd = signalfd.init()
		asyncore.file_dispatcher.__init__(self, fd, map)

	def writable(self):
		return False

	def handle_read(self):
		fail = True
		try:
			signum = signalfd.read()
			if signum > 0:
				self.handle_signal(signum)
			fail = False
		finally:
			if fail:
				self.handle_close()

	def handle_signal(self, signum):
		if signum == signal.SIGINT:
			raise KeyboardInterrupt

	def handle_close(self):
		self.close()
		signalfd.reset()
