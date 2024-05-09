/************************************************************

  This example shows how to read and write data to a dataset
  using the deltaRice compression routine. It tests the routines
  stability across all possible input values of 16-bit integers.

 ************************************************************/

#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>

#define FILE            "test.h5"
#define DATASET         "test"
#define DIM0            65536
#define DIM1            10
#define CHUNK0          32768 
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
    
    const unsigned cd_values[2] = {8, 32768}; //first value is the M parameter, second is the length of the chunk
    short             wdata[DIM0][DIM1],          /* Write buffer */
                    rdata[DIM0][DIM1],          /* Read buffer */
                    maxr, maxw,
                    i, j;

    status = deltarice_register_h5filter();
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

    for (i=0; i<DIM0; i++)
        for (j=0; j<DIM1; j++)
            wdata[i][j] = i;
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
        case H5Z_FILTER_DELTARICE:
            printf ("H5Z_FILTER_DELTARICE\n");
            break;
        default:
            printf ("Not DeltaRice as expected\n");
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
