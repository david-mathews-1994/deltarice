import h5py
import deltaRice.h5
import numpy as np
import time

RiceParameter=8
WaveformLength=100000
compression_opts = (RiceParameter, WaveformLength)
dtype=np.int16
fullsize = (1000, 1000, 1000)
chunksize = (10, 1000, 1000)

f = h5py.File('testFile.h5', 'w')
'''
dataset = f.create_dataset('data', 
	fullsize, 
	compression=deltaRice.h5.H5FILTER,
	compression_opts=compression_opts,
	dtype=dtype,
	chunks=chunksize)
'''
dataset = f.create_dataset('data', fullsize, compression=deltaRice.h5.H5FILTER, dtype=dtype)
array = np.random.normal(0, 10, size=fullsize).astype(dtype)
dataset[:] = array
f.close()
f = h5py.File('testFile.h5', 'r')
dataset = f['data'][()]

print(np.array_equal(array, dataset))
