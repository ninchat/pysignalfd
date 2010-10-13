__all__ = [
	"init",
	"reset",
	"signal_dispatcher",
]

import asyncore
import signal

from _signalfd import *

class signal_dispatcher(asyncore.file_dispatcher):

	def __init__(self):
		fd = init()
		asyncore.file_dispatcher.__init__(self, fd)

	def writable(self):
		return False

	def handle_read(self):
		data = self.recv(1)
		if data:
			if isinstance(data, str):
				signum = ord(data[0])
			else:
				signum = data[0]

			self.handle_signal(signum)
		else:
			self.handle_close()

	def handle_signal(self, signum):
		if signum == signal.SIGINT:
			raise KeyboardInterrupt

	def handle_close(self):
		self.close()
		reset()
