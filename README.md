# Nab HDF5
This is the git repo for the Nab compression algorithm. This algorithm is designed around the compression of signed 16 bit integers and specifically tailored to the types of data being returned by the Nab-DAQ system. 

NabHDF5 contains a Python/C package that implements this algorithm through HDF5. It is loaded via the dynamically loaded filters framework. 

We are still waiting on an official HDF5 filter number but are temporarily using 4020. 

The Nab compression algorithm is based around work to be published. The idea is to reduce randomness in the waveforms by using the correlation between subsequent data samples from an analog signal. As each datapoint is inherently correlated, we can reduce the variation in the data stored through filtering such as the [Link](https://en.wikipedia.org/wiki/Delta_encoding). Through this encoding we can reduce the number of unique symbols per waveform that need to be recorded, or in other words, make the spread of values to be stored more compact and centralized around 0. At this stage the algorithm uses [Link](https://en.wikipedia.org/wiki/Golomb_coding), in particular the special case of Rice Coding, to compress the data. Rice coding was chosen as it is algorithmically simpler and allows for higher throughput. 

## Installation for Python
Installation requires Python 3.3+, HDF5 1.8.4 or later, HDF5 for python (h5py), Numpy, and Cython. NabHDF5 is linked against HDF5. To use the dynamically loaded HDF5 filter requires HDF5 1.8.11 or later. 

To install:

```
python setup.py install
```

If you get an error about missing source files when building the extensions, try upgrading setuptools. setuptools prior to 0.7 sometimes doesn't work with Cython. 

## Example in Python

```
import h5py
impot nabCompression.h5
import numpy as np

f = h5py.File('testFile.h5', 'w')

RiceParamter = 8 #must be positive power of 2 and >= 1
WaveformLength = 7000
compression_opts = (RiceParameter, WaveformLength) #required input parameters
dtype='int16' #THIS IS A MUST. DATA CANNOT BE ANY OTHER TYPE
dataset = f.create_dataset('data', 
    (100, 7000), 
    compression=nabCompression.h5.H5FILTER,
    compression_opts = compression_opts,
    dtype=dtype)

array = np.random.normal(0, 10, size=(100, 7000)).astype(dtype)

dataset[:] = array

f.close()
```
