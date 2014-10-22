[![status](https://travis-ci.org/ninchat/pysignalfd.svg)](https://travis-ci.org/ninchat/pysignalfd)

	import errno
	import select
	import signal

	import signalfd

	fd = signalfd.init()

	while 1:
		try:
			r, w, x = select.select([fd], [], [])
		except select.error as e:
			if e.args[0] == errno.EINTR:
				continue
			raise

		if r and signalfd.read() == signal.SIGTERM:
			break
