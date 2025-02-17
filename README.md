# Hnswlib - fast approximate nearset neighbor search

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
Please refer to [ALGO_PARAMS.md](ALGO_PARAMS.md)
