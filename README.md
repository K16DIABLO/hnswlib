# ADA-NNS (HNSW) - Angular Distance-Guided Neighbor Selection for Graph-Based Approximate Nearest Neighbor Search

This repository is HNSW with ADA-NNS

### Prerequisites

+ GCC 9.4.0+
+ CMake 3.22.2+
+ Eigen library

**IMPORTANT NOTE: this code uses AVX-256 intructions for fast approximate similarity computation, so your machine MUST support AVX-256 intructions, this can be checked using `cat /proc/cpuinfo | grep avx2`.** 

### Datasets

| Name     | Dimension | No. of base | No. of query | Metric |
|----------|-----------|-------------|--------------|--------|
| [SIFT1M](http://corpus-texmex.irisa.fr/)   | 128       | 1,000,000   | 10,000       | L2 |
| [GIST1M](http://corpus-texmex.irisa.fr/)   | 960       | 1,000,000   | 1,000        | L2 |
| [CRAWL](http://github.com/ZJULearning/SSG)    | 300       | 1,989,995   | 10,000       | L2 |
| [GLOVE-100](https://github.com/erikbern/ann-benchmarks)   | 100       | 1,000,000   | 10,000        | IP |
| [NYTIMES](https://github.com/erikbern/ann-benchmarks)   | 256       | 290,000   | 10,000        | IP |
| DEEP100M* | 96        | 100,000,000 | 10,000        | L2 |
+ For DEEP100M, we will share the file link upon request

### Dataset Conversion

For datasets provided in HDF5 format (e.g., GLOVE-100 and NYTIMES):
Parse HDF5 and generate fvecs and ivecs:
```bash
python ./utils/hdf5_to_vecs.py [hdf5_file_name]
mkdir -p dataset/[dataset_name]
mv [dataset_name]_*vecs dataset/[dataset_name]
```


### Compile On Ubuntu

Install Dependencies:

```shell
$ sudo apt-get install g++ cmake libgoogle-perftools-dev libeigen3-dev
```

### Million-scale tests reproduction in WWW'2025 paper
Download datasets in the `dataset` directory
To compile:
```bash
mkdir build
cd build
cmake .. && make -j
```

To reproduce baseline HNSW results:
```bash
./main_1m
```

Parameters are hardcoded in **1m_test.cpp**  
Please refer to [ALGO_PARAMS.md](ALGO_PARAMS.md) to get details about parameters
