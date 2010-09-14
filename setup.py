import distutils.core

ext = distutils.core.Extension(
	"signalfd",
	sources = ["signalfd.c"],
)

distutils.core.setup(
	name = "pysignalfd",
	ext_modules = [ext],
)
