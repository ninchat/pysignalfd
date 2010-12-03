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
		signalfd.reset()
