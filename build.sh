h5cc -std=c99 -fPIC -shared -o nabCompression.so nabCompression.c
h5cc -std=c99 -fPIC -shared -o hdf5_dl.so hdf5_dl.c
h5cc -fPIC -o test testCode.c nabCompression.o
#h5cc -std=c99 -fPIC -shared -o nabCompression.so nabCompression.o
