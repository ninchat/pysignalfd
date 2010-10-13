import distutils.core

ext = distutils.core.Extension(
	"_signalfd",
	sources = ["_signalfd.c"],
)

distutils.core.setup(
	name = "pysignalfd",
	py_modules = ["signalfd"],
	ext_modules = [ext],
)
