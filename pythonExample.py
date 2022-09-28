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
array = np.random.normal(0, 10, size=fullsize).astype(dtype)
start=time.time()
dataset = f.create_dataset('data', fullsize, compression=deltaRice.h5.H5FILTER, dtype=dtype, chunks = chunksize, compression_opts = compression_opts)
dataset[:] = array
f.close()
print(time.time()-start)

f = h5py.File('testFile.h5', 'r')
start = time.time()
dataset = f['data'][()]
print(time.time()-start)

print(np.array_equal(dataset, array))

