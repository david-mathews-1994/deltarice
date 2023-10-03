---
title: 'Gala: A Python package for galactic dynamics'
tags:
  - h5py
  - HDF5
  - compression
  - digitization
  - GPU
authors:
  - name: David Mathews
    orcid: 0000-0002-4897-4379
    affiliation: "1, 2" # (Multiple affiliations must be quoted)
    corresponding: true 
affiliations:
 - name: Oak Ridge National Laboratory
   index: 1
 - name: University of Kentucky
   index: 2
date: 3 October 2023
bibliography: paper.bib

# Optional fields if submitting to a AAS journal too, see this blog post:
# https://blog.joss.theoj.org/2018/12/a-new-collaboration-with-aas-publishing
aas-doi: 10.3847/xxxxx <- update this with the DOI from AAS once you know it.
aas-journal: Astrophysical Journal <- The name of the AAS journal.
---

# Summary

An efficient method for high performance lossless compression of digitized analog signals within the HDF5 library and data format is presented. This algorithm, called Delta-Rice, was developed for the Nab experiment, a fundamental neutron physics experiment, but is broadly applicable to other experimental efforts requiring lossless compression of noisy data. This is a two-step process where correlations between consecutive datapoints are reduced before being passed to a traditional Rice compression algorithm. The reduction of correlations can be tuned to match each dataset through adjustment of the pre-processing filter. A routine for determining the optimal filter for a dataset is demonstrated. Also demonstrated are modifications to allow deployment of this algorithm to both FPGA and GPU architectures. When applied to data collected by the Nab data acquisition system, this method created output files which were 29% their initial size, and was able to do so with an average read/write throughput in excess of 2 GB/s on a single CPU. Compared to the widely used G-zip compression routine, Delta-Rice reduces the file size by 33% more with over an order of magnitude increase in read/write throughput. Delta-Rice is available to users through the HDF5 library.

# Introduction

Being able to digitize, process, and record analog signals is a critical part of many modern physics experiments. For large-scale or high-statistics experiments in particular, the resulting datasets can require petabytes of storage leading to difficulties in both storage and data distribution. The Nab experiment, which will precisely characterize the decay of free neutrons, is one such high-statistics experiment [@Fry2019]. During the multi-year experiment, $10^9$ events will be measured with a data acquisition (DAQ) system recording between $100$ and $400$ MB/s of digitized detector data [@david_2022]. The resulting dataset is expected to reach several petabytes in size. The cost and complexity of storing and distributing this dataset to collaborators motivated the development of a custom compression routine. 

The data size reduction algorithm must meet three primary requirements. First, the algorithm needs to be lossless. Data reconstructed from the compressed files must be equivalent to the input data. Second, it must be able to compress data faster than the data are acquired. This allows the compression to be performed in real-time as part of the data acquisition pipeline reducing the local storage requirements and negating the need for a buffer space to hold data pre-compression. Third, it must be easily accessible to all users and support multiple analysis methodologies, programming languages, and hardware platforms including GPUs. While software such as Gzip [@gzip] or 7-Zip [@7zip] is easily accessible and can significantly reduce the size of a dataset, the throughput of these methods is not at the level required for real time compression.

The Delta-Rice algorithm described in this paper offers real-time lossless compression/decompression that is tailored to Nab experimental dataset. This algorithm is adjustable to other experimental datasets and designed to be compatible with CPU, GPU, and FPGA architectures. A parallel CPU implementation of this compression software is available through the HDF5 library [@hdf5].

# Algorithm Overview
This algorithm is a two-step process: the digitized signal is first passed through an encoding operation with the intention of de-correlating the data and preparing it for the second step of Golomb coding [@golomb]. This two-step process is effective for several reasons. First, both the encoding operation and Golomb coding are computationally simple algorithms that can be easily deployed to CPUs, GPUs, and FPGAs. Secondly, subsequent samples from digitized analog signals are inherently correlated with each other. These correlations can be exploited to better prepare the data for Golomb Coding resulting in a greater level of compression. In the subsequent sections we describe the Golomb Coding operation and the resulting requirements placed on the preparatory encoding filter.

## Golomb Coding
The requirements for the first-state preparatory encoding are driven by the behavior of Golomb coding, described here. Golomb coding was chosen for this compression routine specifically for its simplicity, throughput, and storage efficiency. In addition it does not require a significant amount of additional information to be stored alongside the compressed data in order for the decompression routine to function which improves storage efficiency further. This algorithm can be easily deployed on both GPUs and FPGAs with minimal adjustments/optimizations required to reach peak throughput. 

Golomb coding functions by encoding a value $x$ in 2 pieces: $q$, the result of a division by a tunable parameter $m$, and $r$, the remainder of that division. Golomb coding expects unsigned values, but can be extended to support signed numbers by interleaving positive and negative values. This can be implemented as shown below.

 - if x >= 0:
   -  xPrime = 2*x;
- else:
  - xPrime = 2*abs(x)-1;
- q = xPrime//m;
- r = xPrime - q*m;

The parameter $q$ is stored in Unary coding, and $r$ is stored in truncated binary encoding. The optimal $m$ depends on the standard deviation of the data being compressed, and is determined empirically to yield the maximum compression ratio. Unary coding takes $q+1$ bits to store $q$, while the storage of $r$ takes either $s-1$ or $s$ bits, where $s=\lceil \log_2(m) \rceil$, if $0\leq r<2^{s}-m$ or if $2^{s}-m \leq r < m$ respectively. In the simpler case of Rice coding [@rice], the algorithm is the same except $m$ is restricted to only powers of 2. This simplifies the storage of the remainder $r$ to only needing $s$ bits. In both Golomb and Rice coding, large $q$ values can be problematic due to using Unary coding possibly taking more space than the original datapoint $x$. If $\lfloor 2 \rfloor $ test.
