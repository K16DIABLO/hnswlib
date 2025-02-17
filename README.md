# Hnswlib - fast approximate nearset neighbor search

### Prerequisites

+ GCC 9.4.0+
+ CMake 3.22.2+

### Datasets

| Name     | Dimension | No. of base | No. of query | Metric |
|----------|-----------|-------------|--------------|--------|
| [SIFT1M](http://corpus-texmex.irisa.fr/)   | 128       | 1,000,000   | 10,000       | L2 |
| [GIST1M](http://corpus-texmex.irisa.fr/)   | 960       | 1,000,000   | 1,000        | L2 |
| [CRAWL](http://github.com/ZJULearning/SSG)    | 300       | 1,989,995   | 10,000       | L2 |
| [GLOVE-100](https://github.com/erikbern/ann-benchmarks)   | 100       | 1,183,514   | 10,000        | IP |
| [NYTIMES](https://github.com/erikbern/ann-benchmarks)   | 256       | 290,000   | 10,000        | IP |
| DEEP100M* | 96        | 100,000,000 | 10,000        | L2 |
+ For DEEP100M, we will share the file link upon request

### Dataset Conversion

For datasets provided in HDF5 format (e.g., GLOVE-100 and NYTIMES),  
Parse HDF5 and generate fvecs and ivecs as follows:  
```bash
python ./utils/hdf5_to_vecs.py [hdf5_file_name]
mkdir -p dataset/[dataset_name]
mv [dataset_name]_*vecs dataset/[dataset_name]
```

### HNSW Index Parameters

The parameters used to build each HNSW index is as follows:

| Dataset          | ef_construction   | M     |
|----------|-----------|-------------|
| SIFT1M      | 600  | 25   |
| GIST1M      | 800  | 35   |
| CRAWL       | 1000 | 20   |
| GLOVE-100   | 2500  | 25   |
| NYTIMES      | 500 | 16   |
| DEEP100M       | 800  | 40   |

These parameters are hardcoded in **1m_test.cpp**  
Please refer to [ALGO_PARAMS.md](ALGO_PARAMS.md) to get details about parameters to construct HNSW index

### Compile On Ubuntu

Install Dependencies:

```shell
$ sudo apt-get install g++ cmake libgoogle-perftools-dev libeigen3-dev
```

### Million-scale Tests Reproduction in WWW'2025 Paper
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
