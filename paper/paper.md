---
title: 'Delta-Rice: A HDF5 Compression Plugin optimized for Digitized Detector Data' 
tags:
  - h5py
  - HDF5
  - compression
  - digitization
  - GPU
authors:
  - name: D. G. Mathews 
    orcid: 0000-0002-4897-4379
    affiliation: "1, 2" # (Multiple affiliations must be quoted)
    corresponding: true
  - name: C. B. Crawford
    orcid: 0000-0002-1932-4334
    affiliation: "2" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: S. Baeßler
    orcid: 0000-0001-7732-9873
    affiliation: "1, 3" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: N. Birge
    orcid: 0000-0003-1894-5494
    affiliation: "4" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: L. J. Broussard
    orcid: 0000-0001-9182-2808
    affiliation: "1" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: F. Gonzalez
    orcid: 0000-0002-5954-4155
    affiliation: "1" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: L. Hayen
    orcid: 0000-0002-9471-0964
    affiliation: "5, 6, 7" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: A. Jezghani
    orcid: 0000-0002-4302-4227
    affiliation: "2, 8" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: H. Li
    orcid: 0000-0003-3726-9663
    affiliation: "3"
    corresponding: false
  - name: R. Mammei
    orcid: 0009-0005-3481-4832
    affiliation: "9, 10" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: A. Mendelsohn
    orcid: 0000-0002-4847-2133
    affiliation: "9" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: G. Randall
    orcid: 0000-0002-9713-8465
    affiliation: "11" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: G. V. Riley
    orcid: 0000-0001-7323-8448
    affiliation: "12" # (Multiple affiliations must be quoted)
    corresponding: false
  - name: D. C. Schaper
    orcid: 0000-0002-6219-650X
    affiliation: "2, 12" # (Multiple affiliations must be quoted)
    corresponding: false
  
  
affiliations:
 - name: Oak Ridge National Laboratory
   index: 1
 - name: University of Kentucky
   index: 2
 - name: University of Virginia
   index: 3
 - name: University of Tennessee
   index: 4
 - name: North Carolina State University
   index: 5
 - name: Triangle Universities Nuclear Laboratory
   index: 6
 - name: Normandie University
   index: 7
 - name: Georgia Institute of Technology
   index: 8
 - name: University of Manitoba
   index: 9
 - name: University of Winnipeg
   index: 10
 - name: Arizona State University
   index: 11
 - name: Los Alamos National Laboratory
   index: 12
 
   
date: 3 October 2023
bibliography: paper.bib
---

# Summary

Delta-Rice is an HDF5 [@hdf5] filter plugin that was developed to compress digitized detector signals recorded by the Nab experiment [@Fry2019], a fundamental neutron physics experiment. This is a two-step process where incoming data is passed through a pre-processing filter and then compressed with Rice coding. A routine for determining the optimal pre-processing filter for a dataset is provided along with an example GPU deployment. When applied to data collected by the Nab data acquisition system, this method produced output files 29% their initial size, and was able to do so with an average read/write throughput in excess of 2 GB/s on a single CPU. Compared to the widely used Gzip compression routine, Delta-Rice reduces the file size by 33% more with over an order of magnitude increase in read/write throughput. Delta-Rice is available on CPU to users through the HDF5 library.

# Statement of Need

Many modern nuclear physics experiments, such as the Nab experiment, will produce petabytes of data. The cost and complexity of storing such a datasets motivated the development of a compression routine tailored specifically to the type of signals commonly recorded in these experiments. In these experiments, any compression routine must be fast enough to support real-time compression while also being lossless to prevent any reduction in the precision of offline analysis. Additionally, any candidate routine must be easily accessible to the various members of the collaboration and should not restrict users to a particular programming language to allow for a variety of analysis methods. `Delta-Rice' was designed to meet these requirements and was implemented as an HDF5 filter plugin to ensure that each user can easily access data with minimal additional requirements in multiple programming languages [@david_2022]. While many other filter plugins exist for HDF5 files, such as Bitshuffle [@MASUI2015181] and Gzip, Delta-Rice offers improved throughput and reduction in data size for many experimental efforts such as the Nab, NOPTREX [@noptrex], and nEDM@SNS [@nedmsns] efforts. 

# Algorithm Overview
This algorithm is a two-step process: the digitized signal is first passed through an encoding operation, such as delta encoding, to de-correlate the data and prepare it for the second step of Rice coding [@rice]. These methods were chosen for this compression routine specifically for their simplicity, throughput, and storage efficiency. They also do not require a significant amount of additional information to be stored alongside the compressed data in order for the decompression routine to function, which improves storage efficiency further. 

## Rice Coding 
Rice coding functions by encoding a value $x$ in 2 pieces: $q$, the result of a division by a tunable parameter $m$, and $r$, the remainder of that division. $q$ is stored in Unary coding, with $r$ in truncated binary. In this routine, signed values are handled by interleaving positive and negative values as follows: $x\prime = 2*x$ for $x>=0$ and $x\prime = 2|x|-1$ for $x<0$. Rice coding is used instead of the more general Golomb coding [@golomb] because the restriction to powers of $2$ for $m$ allows for more efficient calculations. For information about the optimization of $m$, see [Optimization](https://github.com/david-mathews-1994/deltarice/blob/master/docs/Optimization.md). In the case that $q>=8$, the output will be $q=8$ followed by the original number in 16-bit signed representation. This is done to ensure that the amount a value can fail to be compressed is fixed. The outputs from this method are packed sequentially into 32 bit containers ensuring that no bits are wasted for any containers but the last one for a dataset.

![](./../docs/images/BitPackingTable.png?raw=true)

*A demonstration of rice coding and bit packing when writing $x=-2$ and $x=25$ with $m=8$ for a $8$ bit output container with a 16 bit temporary cache. Any remaining data in the temporary buffer is retained for the next write of $x$, or output at the end of the compression when no more values of $x$ are provided.*

## Preparatory Encoding
Preparatory encoding is done to adjust the dataset to a form more optimal for Rice Coding. By default, this is done with delta encoding which stores the difference between subsequent values. The image below shows an example of this when applied to a signal from the Nab experiment. A simple optimization routine for determining the ideal filter is discussed in [Optimization](https://github.com/david-mathews-1994/deltarice/blob/master/docs/Optimization.md). 

![](./../docs/images/ExampleEncoding.png?raw=true)

*Left: A waveform before and after delta encoding. Applying Rice coding with $m=8$ on the original signal expands the size of the waveform from 14 kB to 18.2 kB. The same Rice coding operation on the delta encoded waveform compresses the waveform to 4.6 kB, 33% the original size. Right: A histogram of a sample dataset before and after delta encoding. Note the clear reduction in the distribution width and that the most probable values are centered around 0.*

# Implementation

Delta-Rice is accessible to users through the HDF5 library [@hdf5] as filter ID $32025$. The user can specify $m$, the encoding filter, and the length of the smallest axis of the data being stored $l$. If $l$ is specified and OpenMP [@openmp] is available, then the algorithm will utilize multiple threads to compress/decompress the data. Note that datasets written in parallel can be read by either serial or parallel decoding operations, but a dataset written serially will be read serially unless $l$ was specified. For performance information and a discussion of using this routine on GPUs and FPGAs, see [Performance](https://github.com/david-mathews-1994/deltarice/blob/master/docs/Performance.md). 

# Acknowledgements

This research was sponsored by the U.S. Department of Energy (DOE), Office of Science, Office of Nuclear Physics [contracts DE-AC05-00OR22725, DE-SC0014622, DE-FG02-03ER41258] and National Science Foundation (NSF) [contract PHY-1812367]. This research was also sponsored by the U.S. Department of Energy, Office of Science, Office of Workforce Development for Teachers and Scientists (WDTS) Graduate Student Research (SCGSR) program. This research was supported in part through research cyberinfrastructure resources and services provided by the Partnership for an Advanced Computing Environment (PACE) at the Georgia Institute of Technology, Atlanta, Georgia, USA.

# References


