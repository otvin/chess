from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize


extensions = [
    Extension("checktables", ["check_tables.pyx"])
]
setup(
    ext_modules = cythonize(extensions),
)


setup(
    ext_modules = cythonize("chess_cython.pyx")
)