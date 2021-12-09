import h5py
import nabCompression.h5
import numpy as np

RiceParameter=8
WaveformLength=5
compression_opts = (RiceParameter, WaveformLength)
dtype=np.int16
fullsize = (5, 5)
chunksize = (5, 5)

f = h5py.File('testFile.h5', 'w')
dataset = f.create_dataset('data', 
	fullsize, 
	compression=nabCompression.h5.H5FILTER,
	compression_opts=compression_opts,
	dtype=dtype,
	chunks=chunksize)

array = np.random.normal(0, 10, size=fullsize).astype(dtype)

dataset[:] = array

f.close()

f = h5py.File('testFile.h5', 'r')
dataset = f['data']

raw_bytes = dataset.id.read_direct_chunk((0, 0))[1]

buffer = np.frombuffer(raw_bytes, dtype=np.int8)
waveformBit = buffer.view(np.int16)[12:].view(np.uint32)

print(np.array_equal(dataset[()], array))
