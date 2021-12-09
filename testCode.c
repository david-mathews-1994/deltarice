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
