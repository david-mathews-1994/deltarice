import h5py
import deltaRice.h5
import numpy as np

RiceParameter=8
WaveformLength=5
compression_opts = (RiceParameter, WaveformLength)
dtype=np.int16
fullsize = (100, 100, 100)
chunksize = (10, 10, 10)

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
dataset = f['data']

print(np.array_equal(dataset[()], array))
