# Performance

The performance of the algorithm was tested against several other compression routines to determine which is best suited for deployment in real-time data acquisition and analysis systems. Both the reduction in size and the rate of the compression were compared. All tests, unless otherwise stated, were performed on the machine described here. A RAM disk was used for the storage allocation to allow for peak throughput and reduce the effect of disk I/O on the results. 

 - OS: Ubuntu 22.04
 - CPU: AMD Threadripper Pro 5955WX
 - RAM: 4x32 GB DDR4 @ 3200 MHz
 - Storage: tmpfs

The competing algorithms used during testing are all commonly available in HDF5 implementations: Gzip, LZF, and Bitshuffle. Testing was performed in Python using the h5py library to manage the reading and writing from the HDF5 file [@h5py]. Each test was performed 10 times to reduce random variance in the results and the mean of these tests is reported. For all testing, the initial data were stored in memory before being written to the file. All data were written to a ramdisk configured with tmpfs to reduce dependence on hard-drive read/write performance. An additional set of benchmarks were done without compression enabled to establish the baseline file I/O performance in the system for each dataset. The Delta-Rice algorithm was run in both single-threaded and multi-threaded mode to show the scaling performance of the algorithm and used the default $m=8$ parameter and the delta encoding filter for all datasets. Gzip, LZF, and Bitshuffle were all run with the default parameters. Each routine was run with the same chunksize, the dimensions of the uncompressed data in memory, to keep the chunking behavior consistent. Recorded signals from the Nab and NOPTREX experiments were used to test this routine, along with simulated detector signals from the nEDM@SNS experiment [@nedmsns]. The figure below shows examples of these signals. Each signal type has very different shape features and lengths deliberately to test the flexibility of the algorithms. 

![](./images/ComparingSignalFeatures.png?raw=true)

*Comparing the features of the different signal types from Nab, nEDM@SNS, and NOPTREX. Each dataset has significantly different signal shapes and features. Both Nab and NOPTREX share exponentially decaying signal shapes, but with opposite polarities and different noise profiles while nEDM@SNS is a square pulse.*

The first results are for the Nab experiment dataset. All methods used the same chunksize of $2000\times7000$ and the segment length in Delta-Rice was set to 7000. 

| Compression Method      | File Size (%) | Read (MB/s) | Write (MB/s) |
|-------------------------|---------------|-------------|--------------|
| No Compression          | 100           | 3587        | 2906         |
| Gzip                    | 39            | 374         | 71           |
| LZF                     | 59            | 717         | 554          |
| Bitshuffle (32 threads) | 38            | 1852        | 2131         |
| Delta-Rice (1-thread)   | 29            | 229         | 500          |
| Delta-Rice (32-thread)  | 29            | 1782        | 2387         |

The second set of tests was run on simulated signals from the nEDM@SNS experiment. All methods used a chunksize of $32\times81920$ and a segment length of $81920$ was set for the Delta-Rice method. 

| Compression Method      | File Size (%) | Read (MB/s) | Write (MB/s) |
|-------------------------|---------------|-------------|--------------|
| No Compression          | 100           | 3759        | 3799         |
| Gzip                    | 29            | 440         | 75           |
| LZF                     | 49            | 722         | 662          |
| Bitshuffle (32 threads) | 30            | 1833        | 1353         |
| Delta-Rice (1-thread)   | 27            | 436         | 539          |
| Delta-Rice (32-thread)  | 27            | 1717        | 2084         |

The third and final set of testing was performed on data from the NOPTREX collaboration. All methods used a chunksize of $32\times500000$ with a segment length of $500000$ specified for the Delta-Rice method. 

| Compression Method      | File Size (%) | Read (MB/s) | Write (MB/s) |
|-------------------------|---------------|-------------|--------------|
| No Compression          | 100           | 3792        | 2829         |
| Gzip                    | 22            | 472         | 75           |
| LZF                     | 44            | 767         | 670          |
| Bitshuffle (32 threads) | 26            | 1201        | 2754         |
| Delta-Rice (1-thread)   | 25            | 435         | 855          |
| Delta-Rice (32-thread)  | 25            | 2235        | 2548         |

For these datasets, the Delta-Rice algorithm either matches or outperforms the other algorithms in both file size reduction and read/write throughput. The only case where Delta-Rice does not offer the best size reduction is for the NOPTREX dataset where Gzip reduced the file size 3% more, but at the cost of several times slower read/write performance. The Bitshuffle algorithm was the closest competitor in these tests but was consistently producing a few percent larger files. Note that in nearly all cases, the compression performance was greater than the decompression performance for Delta-Rice. This is primarily due to the implementation of Rice coding being more efficient for compressing data than re-constructing the compressed data. 

## GPU Performance Testing

For testing GPU performance a different computer was used and its specifications are below. 

 - OS: Red Hat Enterprise 7.9
 - CPU: AMD EPYC 7513 
 - RAM: 512GB DDR4
 - Storage: tmpfs
 - GPU: Nvidia A100 80GB

The most straightforward way of implementing this routine on GPU is by compressing/decompressing one chunk of data in parallel, similar to how it is performed on CPU. While it is possible to handle multiple chunks at once, this was not done during testing to keep the configuration as similar as possible to the CPU tests. For a chunk that is $2000\times7000$ as for the Nab dataset testing before, that would require $2000$ independent threads operating in parallel on a GPU for both the compression and decompression operations. Depending on the GPU in particular being used, that may be either too many or too few depending on the number of compute units available in the system. Tuning will need to be performed on a per-GPU basis to optimize the chunk size for throughput. 

For this testing, a few different chunk sizes were used to demonstrate this on the Nab dataset. The table below is for the same chunk size of $2000\times7000$ that was used previously on the Nab dataset. Only multi-threaded CPU performance is shown below. Note that File in these tests denotes the source or target was the HDF5 file stored using tmpfs whereas RAM denotes the data started or stopped as an array in memory outside of the HDF5 file structure.

| Method               | Data Source Location | Target Location | Throughput |
|----------------------|----------------------|-----------------|------------|
| CPU No Compression   | RAM                  | File            | 6000 MB/s  |
| CPU No Compression   | File                 | RAM             | 7100 MB/s  |
| CPU With Compression | RAM                  | File            | 1250 MB/s  |
| CPU With Compression | File                 | RAM             | 2050 MB/s  |
| GPU With Compression | RAM                  | File            | 1210 MB/s  |
| GPU With Compression | File                 | RAM             | 2150 MB/s  |
| GPU With Compression | VRAM                 | File            | 2550 MB/s  |
| GPU With Compression | File                 | VRAM            | 3375 MB/s  |

In this particular set of tests, the GPU compression/decompression performance was roughly the same when the data source or destination were not on the GPU. These cases require an additional data transfer which reduces throughput. The highest throughput case was when the data is read compressed from the file, decompressed on the GPU, and remains on the GPU. This is because the total amount of data transfered to and from VRAM is the smallest of all cases. Increasing the chunk size to $20000\times7000$ improved performance of the compression routine across the board due to allowing for more parallel instances at once as shown below.

| Method               | Data Source Location | Target Location | Throughput |
|----------------------|----------------------|-----------------|------------|
| CPU No Compression   | RAM                  | File            | 5850 MB/s  |
| CPU No Compression   | File                 | RAM             | 7050 MB/s  |
| CPU With Compression | RAM                  | File            | 2850 MB/s  |
| CPU With Compression | File                 | RAM             | 2550 MB/s  |
| GPU With Compression | RAM                  | File            | 1300 MB/s  |
| GPU With Compression | File                 | RAM             | 2450 MB/s  |
| GPU With Compression | VRAM                 | File            | 2800 MB/s  |
| GPU With Compression | File                 | VRAM            | 5900 MB/s  |

The GPU compression and decompression performance truly shines when the data originates or has its final destination on the GPU. When the situation requires transfers to and from the GPU, the performance is significantly lower, and in general the multi-threaded CPU implementation is a better choice. However, if a user is in a situation where they are reading data from a file with the intent of processing on GPU, this routine can significantly improve the read performance to nearly the full throughput of an uncompressed data file. 

## FPGA Performance Testing

Unlike the CPU and GPU, FPGAs are deterministic devices that will perform a fixed set of operations every clock cycle. As such, the clock frequency of the compression or decompression function will determine throughput of the routine. For example, if the algorithm is compiled to run at 125 MHz, then the throughput of the code will be $\approx 238$ MB/s as every clock cycle will compress a single $16$ bit number. This compression routine, using the delta encoding filter, was synthesized for a single channel of the NI PXIe 5171 Oscilloscope Modules used in the Nab experiment at a rate of $250$ MHz, or $\approx 480$MB/s. By having the FPGA compress data from multiple channels simultaneously, it is possible for this rate to be improved even further, but the number of simultaneous streams and maximum frequency will depend on the FPGA. 
