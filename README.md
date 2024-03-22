# Delta-Rice
## IMPORTANT NOTE
In a recent git commit the format of the compressed data was tweaked. Users should be aware that any data saved with the prior version can NOT be read by the new version and should be decompressed before upgrading their version of the library.

## Main Information
This is the git repo for the Delta-Rice compression algorithm. This algorithm is designed around the compression of signed 16 bit integers and specifically tailored to the types of data being returned by the Nab Experiments Data Acquisition System: http://nab.phys.virginia.edu/. The algorithm itself is general purpose however and can work on a variety of datasets, but it does expect 16-bit numbers. Any other precision data is cast to 16-bit. 

Delta-Rice contains a Python/C package that implements this algorithm through HDF5. It is loaded via the dynamically loaded filters framework and uses filter ID 32025.

The Delta-Rice compression algorithm is based around work to be published. The idea is to prepare the dataset for Rice coding by using the correlation between subsequent data samples from an analog signal. As each datapoint is inherently correlated with the previous datapoint, we can reduce the variation in the data stored through filtering such as the [Link](https://en.wikipedia.org/wiki/Delta_encoding). Through this encoding operation, the number of unique symbols per waveform that need to be recorded can be reduced, or in other words, make the spread of values to be stored more compact and centralized around 0. At this stage the algorithm uses [Link](https://en.wikipedia.org/wiki/Golomb_coding), in particular the special case of Rice Coding, to compress the data. Rice coding was chosen as it is algorithmically simpler than Golomb coding and allows for higher throughput and during testing Golomb coding was not found to provide a significant decrease in size over Rice coding. 

A significant portion of this code repository, in particular the code that does the installation of the library but not the compression algorithm itself, is based off of code found here: https://github.com/kiyo-masui/bitshuffle. 

## Installation instructions for Linux
Installation requires Python 3.3+, HDF5 1.8.4 or later, HDF5 for python (h5py), Numpy, and Cython. Delta-Rice is linked against HDF5. To use the dynamically loaded HDF5 filter requires HDF5 1.8.11 or later. 

To install:

```
python setup.py install
```

If you get an error about missing source files when building the extensions, try upgrading setuptools. setuptools prior to 0.7 sometimes doesn't work with Cython. 

This has been tested and verified to work on Ubuntu 20.04 and WSL Ubuntu.

## Installation instructions for Windows
Installation requires Python 3.3+, HDF5 1.8.4 or later, HDF5 for python (h5py), Numpy, and Cython. Delta-Rice is linked against HDF5. To use the dynamically loaded HDF5 filter requires HDF5 1.8.11 or later. 

To install you first need to install the HDF5 library. I did this process through conda using Anaconda prompt.

```
conda install -c anaconda hdf5
```
With hdf5 installed, find the installation location. The default installation location should be beneath the anaconda3/pkgs folder on your system. With this installation location found, you need to open the setup.py file and modify the initial definition of the FALLBACK_CONFIG variable to match your system. An example for my computer is shown below.  

```
FALLBACK_CONFIG = {
    "include_dirs": ["C:/Users/David/anaconda3/pkgs/hdf5-1.12.0-h1756f20_0/Library/include"],
    "library_dirs": ["C:/Users/David/anaconda3/pkgs/hdf5-1.12.0-h1756f20_0/Library/lib"],
    "libraries": ["C:/Users/David/anaconda3/pkgs/hdf5-1.12.0-h1756f20_0/Library/lib"],
    "extra_compile_args": [],
    "extra_link_args": [],
}
```
With this modification complete, close the setup.py file and run the setup.py script as shown below.

```
python setup.py install
```

This will most likely NOT succeed, and that is okay. On my system I see the following error that causes the code to fail with exit status 1120.
```
LINK : error LNK2001: unresolved external symbol PyInit_libh5deltaricecompression
```
If this error is reached, then the code actually compiled just fine for our purposes. Run the ```python setup.py install``` command a second time and you should see no errors. If you do see errors the second time, then something went wrong. In particular, if you get an error about missing source files when building the extensions, try upgrading setuptools. I have noticed compatibility issues with setuptools prior to 0.7 not working with Cython. 

## Example in Python

This example goes through and creates a dataset using the custom deltaRice compression algorithm, then re-opens the file and reads in the same dataset verifying it compressed and decompressed properly. It's important to note that when using h5py and the create_dataset function that the chunksize MUST be specified. This must be specified such that the length of each waveform is the same size as the smallest dimension in the chunk. For a waveform length of 7000, this means that the chunks must be configured as (Something, 7000). If this is not done, the code will return an error. 

Note that with this particular example the delta-rice library, in particular the deltaRice folder in the build directory, has been added to the system path allowing Python to include it.

```
import h5py
import deltaRice.h5
import numpy as np

f = h5py.File('testFile.h5', 'w')

RiceParameter = 8 #must be positive power of 2 and >= 1
WaveformLength = 7000
compression_opts = (RiceParameter, WaveformLength) #required input parameters
dtype='int16' #THIS IS A MUST. DATA CANNOT BE ANY OTHER TYPE
dataset = f.create_dataset('data', 
    (100, 7000), 
    compression=deltaRice.h5.H5FILTER,
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

This example is similar to the Python example. We create a dataset, compress it with the delta-rice compression algorithm, then close the file. The code then re-opens the file, reads in the data, and verifies it compressed and decompressed properly. 

### Raw Code

```
#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>

#define FILE            "test.h5"
#define DATASET         "test"
#define DIM0            5
#define DIM1            5
#define CHUNK0          5 
#define CHUNK1          5
#define H5Z_FILTER_DELTARICE  32025

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
    const unsigned cd_values[2] = {8, 5};
    short             wdata[DIM0][DIM1], 
                    rdata[DIM0][DIM1], 
                    i, j;

    /* 
       Register deltarice filter filter with the library
     */

    status = deltarice_register_h5filter();
    /*
    Check to see if the filter properly registered to the HDF5 Library
     */
    avail = H5Zfilter_avail(H5Z_FILTER_DELTARICE);
    if (!avail) {
        printf ("deltarice filter not available.\n");
        return 1;
    }
    status = H5Zget_filter_info (H5Z_FILTER_DELTARICE, &filter_info);
    if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ||
                !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) ) {
        printf ("deltarice filter not available for encoding and decoding.\n");
        return 1;
    }

    /*
     * Initialize data and find its maximum value to check later.
     */
    for (i=0; i<DIM0; i++)
        for (j=0; j<DIM1; j++)
            wdata[i][j] = 0;

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
    status = H5Pset_filter (dcpl, H5Z_FILTER_DELTARICE, H5Z_FLAG_MANDATORY, (size_t)2, cd_values);
    status = H5Pset_chunk (dcpl, 2, chunk);

    /*
     * Create the dataset.
     */
    dset = H5Dcreate (file, DATASET, H5T_NATIVE_SHORT, space, H5P_DEFAULT, dcpl,
                H5P_DEFAULT);

    /*
     * Write the data to the dataset.
     */
    status = H5Dwrite (dset, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                wdata[0]);
    /*
     * Close and release resources.
     */
    status = H5Pclose (dcpl);
    status = H5Dclose (dset);
    status = H5Sclose (space);
    status = H5Fclose (file);


    /*
     * Now we begin the read section of this example.
     */

    /*
     * Open file and dataset using the default properties.
     */
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
        case H5Z_FILTER_DELTARICE:
            printf ("H5Z_FILTER_DELTARICE\n");
            break;
        default:
            printf ("Not DELTARICE as expected\n");
    }

    /*
     * Read the data using the default properties.
     */
    status = H5Dread (dset, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                rdata[0]);
    /*
    Verify the data read in is the same as was written.
     */
    bool allMatch = true;
    for (i=0; i<DIM0; i++) {
        for (j=0; j<DIM1; j++)  {
            if(rdata[i][j] != wdata[i][j])
        	allMatch = false;
	    }
    }
    /*
    Print if it all matched
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
h5cc -L/home/dgm/deltaRice/build/lib.linux-x86_64-3.8/deltaRice/plugin -Wall -o testCode testCode.c  -lh5deltaricecompression.cpython-38-x86_64-linux-gnu
```

You will also need to modifiy your `$LD_LIBRARY_PATH` to be the plugin folder beneath the build/ directory. This allows the system to know where to grab the linked library at runtime. This can be done via: `export LD_LIBRARY_PATH=/dir/of/plugin:$LD_LIBRARY_PATH` where `/dir/of/plugin` is wherever your plugin folder is located under the build/ directory from this gitlab. 
