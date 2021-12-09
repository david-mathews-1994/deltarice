"""

Mirror of the Bitshuffle file here: https://github.com/kiyo-masui/bitshuffle/blob/master/bitshuffle/h5.pyx


Constants
=========
    H5FILTER : The Bitshuffle HDF5 filter integer identifier.
Functions
=========
    create_dataset
"""


import sys
import h5py
from h5py import h5d, h5fd, h5s, h5t, h5p, h5z, defs, filters

cimport cython

cdef extern from b"nabCompression.h":
	int nab_register_h5filter()
	int H5Z_FILTER_NAB

cdef extern int init_filter(const char* libname)

H5FILTER = H5Z_FILTER_NAB

# Init HDF5 dynamic loading with HDF5 library used by h5py
if not sys.platform.startswith('win'):
	if sys.version_info[0] >= 3:
		libs = [bytes(h5d.__file__, encoding='utf-8'),
			bytes(h5fd.__file__, encoding='utf-8'),
			bytes(h5s.__file__, encoding='utf-8'),
			bytes(h5t.__file__, encoding='utf-8'),
			bytes(h5p.__file__, encoding='utf-8'),
			bytes(h5z.__file__, encoding='utf-8'),
			bytes(defs.__file__, encoding='utf-8')]
	else:
		libs = [h5d.__file__, h5fd.__file__, h5s.__file__, h5t.__file__,
		h5p.__file__, h5z.__file__, defs.__file__]
	# Ensure all symbols are loaded
	success = -1
	for lib in libs:
		success = init_filter(lib)
		if success == 0:
			break
	if success == -1:
		raise RuntimeError("Failed to load all HDF5 symbols using these libs: {}".format(libs))


def register_h5_filter():
	ret = nab_register_h5filter()
	if ret < 0:
		raise RuntimeError("Failed to register bitshuffle HDF5 filter.", ret)


register_h5_filter()










