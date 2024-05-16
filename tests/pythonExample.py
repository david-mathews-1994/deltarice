import h5py
import deltaRice.h5
import numpy as np
import time


#worst case scenario testing with a bunch of random values
def test_worst_case():
  f = h5py.File('testFile.h5', 'w')
  dset = np.random.uniform(-32768, 32768, size=2**16).astype(np.int16)
  f.create_dataset('test', data = dset, compression=deltaRice.h5.H5FILTER)
  f.close()
  
  f = h5py.File('testFile.h5', 'r')
  test = np.array_equal(f['test'][()], dset)
  f.close()
  assert test, "Failed default random number test"
  return

def test_different_m():
  f = h5py.File('testFile.h5', 'w')
  compression_opts = (16,)
  dset = np.random.uniform(-32768, 32768, size=2**16).astype(np.int16)
  f.create_dataset('test', data = dset, compression=deltaRice.h5.H5FILTER, compression_opts=compression_opts)
  f.close()
  
  f = h5py.File('testFile.h5', 'r')
  test = np.array_equal(f['test'][()], dset)
  f.close()
  assert test, "Failed different M test"
  return

def test_different_m_different_segment_length():
  f = h5py.File('testFile.h5', 'w')
  compression_opts = (8, 1024)
  dset = np.random.uniform(-32768, 32768, size=2**16).astype(np.int16)
  f.create_dataset('test', data = dset, compression=deltaRice.h5.H5FILTER, compression_opts=compression_opts)
  f.close()
  
  f = h5py.File('testFile.h5', 'r')
  test = np.array_equal(f['test'][()], dset)
  f.close()
  assert test, "Failed different M segment length test"
  return

def test_different_filter():
  f = h5py.File('testFile.h5', 'w')
  compression_opts = (8, 1024, 1, 1)
  dset = np.random.uniform(-32768, 32768, size=2**16).astype(np.int16)
  f.create_dataset('test', data = dset, compression=deltaRice.h5.H5FILTER, compression_opts=compression_opts)
  f.close()
  
  f = h5py.File('testFile.h5', 'r')
  test = np.array_equal(f['test'][()], dset)
  f.close()
  assert test, "Failed different filter test"
  return

def test_brute_force_all_signed():
  f = h5py.File('testFile.h5', 'w')
  compression_opts = (8, 1024, 1, 1)
  dset = np.arange(-32768, 32768).astype(np.int16)
  f.create_dataset('test', data = dset, compression=deltaRice.h5.H5FILTER, compression_opts=compression_opts)
  f.close()
  
  f = h5py.File('testFile.h5', 'r')
  test = np.array_equal(f['test'][()], dset)
  f.close()
  assert test, "Failed brute force all possible signed 16-bit values test"
  return

def test_brute_force_all_unsigned():
  f = h5py.File('testFile.h5', 'w')
  compression_opts = (8, 1024, 1, 1)
  dset = np.arange(0, 65536).astype(np.uint16)
  f.create_dataset('test', data = dset, compression=deltaRice.h5.H5FILTER, compression_opts=compression_opts)
  f.close()
  
  f = h5py.File('testFile.h5', 'r')
  test = np.array_equal(f['test'][()], dset)
  f.close()
  assert test, "Failed brute force all possible unsigned 16-bit values test"
  return
