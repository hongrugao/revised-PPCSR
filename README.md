# revised PPCSR
We implement our novel Pg-remap reconstruction approach to improve the reconstruction process of PMA-based dynamic graph processing. Our code is based on an existing work parallel PCSR (https://github.com/domargan/parallel-packed-csr). The corresponding paper "Accelerated Reconstruction of Dynamic Graph Processing with Page Remapping" has been submitted to SIGMOD2025.

# Prerequisites
* CMAKE 3.8 or newer required.

# Build
Create a build directory and run cmake & make there:
```
$ mkdir build && cd build
$ cmake ..
$ make
```
# Running
Run the `parallel-packed-csr` binary from your build directory.

## Command line options
* `-threads=`: specifies number of threads to use for updates, default=8
* `-size=`: specifies number of edges that will be read from the update file, default=1000000
* `-lock_free`: runs the data structure lock-free version of binary search, locks during binary search by default
* `-partitions_per_domain=`: specifies the number of graph partitions per NUMA domain
* `-insert`: inserts the edges from the update file to the core graph
* `-delete`: deletes the edges from the update file from the core graph
* `-core_graph=`: specifies the filename of the core graph
* `-update_file=`: specifies the filename of the update file
* Available partitioning strategies (if multiple strategies are given, the last one is used):
  * `-ppcsr`: No partitioning
  * `-pppcsr`: Partitioning (1 partition per NUMA domain)
  * `-pppcsrnuma`: Partitioning with explicit NUMA optimizations (default)
* `-double_granularity=`: the rg parameter, need to be set as a power to two.

