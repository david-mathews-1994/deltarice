import h5py
import deltaRice.h5
import numpy as np
import time


#create test datasets that span the full range of 16-bit numbers
dset1 = np.arange(-32768, 32768, dtype = np.int16)
dset2 = np.arange(0, 2**16, dtype = np.uint16)
bulkDset = np.zeros(shape = (100, dset2.shape[0]))

bulkOptions = (8, bulkDset.shape[0])

f = h5py.File('testFile.h5', 'w')
f.create_dataset('signed', data = dset1, compression=deltaRice.h5.H5FILTER)
f.create_dataset('unsigned', data = dset2, compression=deltaRice.h5.H5FILTER)
f.create_dataset('bulkDset', data = bulkDset, compression=deltaRice.h5.H5FILTER, compression_opts = bulkOptions)

f.close()

f = h5py.File('testFile.h5', 'r')
test1 = np.array_equal(f['signed'][()], dset1)
test2 = np.array_equal(f['unsigned'][()], dset2)
test3 = np.array_equal(f['bulkDset'][()], bulkDset)


if test1 and test2 and test3:
    print('success!')
    exit(0)
else:
    print('Failure')
    exit(1)

