#ifndef NAB_H5FILTER_H
#define NAB_H5FILTER_H

#define H5Z_class_t_vers 2
#include "hdf5.h"

#define H5Z_FILTER_NAB 4020 //placehold number, not the final value
typedef unsigned long long int superint;

extern H5Z_class_t H5Z_NAB[1];

//declare the filter function 
size_t H5Z_filter_nab(unsigned flags, size_t cd_nelmts, const unsigned cd_values[], size_t nbytes, size_t *buf_size, void**buf);

int nab_register_h5filter(void);

#endif //NAB_H5FILTER_H
