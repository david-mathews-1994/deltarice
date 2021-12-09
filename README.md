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

This example goes through and creates a dataset using the custom Nab compression algorithm, then re-opens the file and reads in the same dataset verifying it compressed and decompressed properly. It's important to note that when using h5py and the create_dataset function that the chunksize MUST be specified. This must be specified such that the length of each waveform is the same size as the smallest dimension in the chunk. For a waveform length of 7000, this means that the chunks must be configured as (Something, 7000). If this is not done, the code will return an error. 

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
    dtype=dtype, 
    chunks = (20, 7000))

array = np.random.normal(0, 10, size=(100, 7000)).astype(dtype)

dataset[:] = array

f.close()

f = h5py.File('testFile.h5', 'r')
dataset = f['data']
readIn = dataset[()]
print(np.array_equal(readIn, array))
```

## Example in C

This example is similar to the Python example. We create a dataset, compress it with the custom Nab compression algorithm, then close the file. The code then re-opens the file, reads in the data, and verifies it compressed and decompressed properly. 

### Raw Code

```
/************************************************************

  Use
       h5cc h5ex_d_bzip2.c H5Zbzip2.c -lbz2 
  command to compile this example.

  This example shows how to read and write data to a dataset
  using bzip2 compression available in the H5Zbzip2.c file that
  accompanies this example.  
  The program first registers bzip2 compression  with the library,
  and then checks if bzip2 compression is available.
  After that it writes integers to a dataset using bzip2,
  then closes the file.  Next, it reopens the file, reads
  back the data, and outputs the type of compression and the
  maximum value in the dataset to the screen.

  This file is intended for use with HDF5 Library version 1.8

 ************************************************************/

#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>

#define FILE            "test.h5"
#define DATASET         "test"
#define DIM0            5
#define DIM1            5
#define CHUNK0          5 
#define CHUNK1          5
#define H5Z_FILTER_NAB  4020

int main (void){
    hid_t           file, space, dset, dcpl;    /* Handles */
    herr_t          status;
    htri_t          avail;
    H5Z_filter_t    filter_type;
    hsize_t         dims[2] = {DIM0, DIM1},
                    chunk[2] = {CHUNK0, CHUNK1};
    size_t          nelmts;
    unsigned int    flags,
                    filter_info;
    const unsigned cd_values[2] = {8, 5};          /* bzip2 default level is 9 */
    short             wdata[DIM0][DIM1],          /* Write buffer */
                    rdata[DIM0][DIM1],          /* Read buffer */
                    maxr, maxw,
                    i, j;

    /* 
       Register bzip2 filter with the library
     */

    status = nab_register_h5filter();
    /*
     * Check if bzip2 compression is available and can be used for both
     * compression and decompression.  Normally we do not perform error
     * checking in these examples for the sake of clarity, but in this
     * case we will make an exception because this filter is an
     * optional part of the hdf5 library.
     */
    avail = H5Zfilter_avail(H5Z_FILTER_NAB);
    if (!avail) {
        printf ("nab filter not available.\n");
        return 1;
    }
    status = H5Zget_filter_info (H5Z_FILTER_NAB, &filter_info);
    if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ||
                !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) ) {
        printf ("nab filter not available for encoding and decoding.\n");
        return 1;
    }

    /*
     * Initialize data and find its maximum value to check later.
     */
    for (i=0; i<DIM0; i++)
        for (j=0; j<DIM1; j++)
            wdata[i][j] = 0;
    maxw = wdata[0][0];
    for (i=0; i<DIM0; i++)
        for (j=0; j<DIM1; j++)
            if (maxw < wdata[i][j])
                maxw = wdata[i][j];

    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    space = H5Screate_simple (2, dims, NULL);

    /*
     * Create the dataset creation property list, add the bzip2 
     * compression filter and set the chunk size.
     */
    dcpl = H5Pcreate (H5P_DATASET_CREATE);
    status = H5Pset_filter (dcpl, H5Z_FILTER_NAB, H5Z_FLAG_MANDATORY, (size_t)2, cd_values);
    status = H5Pset_chunk (dcpl, 2, chunk);

    /*
     * Create the dataset.
     */
    dset = H5Dcreate (file, DATASET, H5T_NATIVE_SHORT, space, H5P_DEFAULT, dcpl,
                H5P_DEFAULT);

    /*
     * Write the data to the dataset.
     */
    printf("pre-writing\n");
    status = H5Dwrite (dset, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                wdata[0]);
    printf("post writing completed\n");
    /*
     * Close and release resources.
     */
    status = H5Pclose (dcpl);
    printf("freed dcpl\n");
    status = H5Dclose (dset);
    printf("freed dset\n");
    status = H5Sclose (space);
    printf("freed space\n");
    status = H5Fclose (file);
    printf("freed file\n");


    /*
     * Now we begin the read section of this example.
     */

    /*
     * Open file and dataset using the default properties.
     */
    printf("writing file completed\n");
    file = H5Fopen (FILE, H5F_ACC_RDONLY, H5P_DEFAULT);
    dset = H5Dopen (file, DATASET, H5P_DEFAULT);

    /*
     * Retrieve dataset creation property list.
     */
    dcpl = H5Dget_create_plist (dset);

    /*
     * Retrieve and print the filter type.  Here we only retrieve the
     * first filter because we know that we only added one filter.
     */
    nelmts = 0;
    filter_type = H5Pget_filter (dcpl, 0, &flags, &nelmts, NULL, 0, NULL,
                &filter_info);
    switch (filter_type) {
        case H5Z_FILTER_NAB:
            printf ("H5Z_FILTER_NAB\n");
            break;
        default:
            printf ("Not NAB as expected\n");
    }

    /*
     * Read the data using the default properties.
     */
    status = H5Dread (dset, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                rdata[0]);
    /*
     * Find the maximum value in the dataset, to verify that it was
     * read correctly.
     */
    bool allMatch = true;
    for (i=0; i<DIM0; i++) {
        for (j=0; j<DIM1; j++)  {
            if(rdata[i][j] != wdata[i][j])
        	allMatch = false;
	}
    }
    /*
     * Print the maximum value.
     */
    printf("Do they all match? ");
    printf("%s\n", allMatch ? "true" : "false");

    /*
     * Close and release resources.
     */
    status = H5Pclose (dcpl);
    status = H5Dclose (dset);
    status = H5Fclose (file);

    return 0;
}
```

### To Compile
In order to compile your code, you need to tell the compiler where to look for the plugin created by the setup.py script. This should be in the build/ folder and saved as a .so file for Linux distributions. The exact folder name will vary with your particular installation features and Python version. 

```
h5cc -L/home/dgm/nabCompression/build/lib.linux-x86_64-3.8/nabCompression/plugin -Wall -o testCode testCode.c  -lh5nabcompression.cpython-38-x86_64-linux-gnu
```

You will also need to modifiy your `$LD_LIBRARY_PATH` to be the plugin folder beneath the build/ directory. This allows the system to know where to grab the linked library at runtime. This can be done via: `export LD_LIBRARY_PATH=/dir/of/plugin:$LD_LIBRARY_PATH` where `/dir/of/plugin` is wherever your plugin folder is located under the build/ directory from this gitlab. 
