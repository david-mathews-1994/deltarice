#ifndef DELTARICE_H5FILTER_H
#define DELTARICE_H5FILTER_H

#define H5Z_class_t_vers 2
#include "hdf5.h"

#define H5Z_FILTER_DELTARICE 32025
typedef unsigned long long int superint;

extern H5Z_class_t H5Z_DELTARICE[1];

//declare the filter function 
size_t H5Z_filter_deltarice(unsigned flags, size_t cd_nelmts, const unsigned cd_values[], size_t nbytes, size_t *buf_size, void**buf);

int deltarice_register_h5filter(void);

#endif //DELTARICE_H5FILTER_H
