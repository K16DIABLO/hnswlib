#include <iostream>
#include <fstream>
#include <queue>
//#include <chrono>
#include "../../hnswlib/hnswlib.h"


#include <unordered_set>

using namespace std;
using namespace hnswlib;

class StopW {
    std::chrono::steady_clock::time_point time_begin;
 public:
    StopW() {
        time_begin = std::chrono::steady_clock::now();
    }

    float getElapsedTimeMicro() {
        std::chrono::steady_clock::time_point time_end = std::chrono::steady_clock::now();
        return (std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_begin).count());
    }

    void reset() {
        time_begin = std::chrono::steady_clock::now();
    }
};



/*
* Author:  David Robert Nadeau
* Site:    http://NadeauSoftware.com/
* License: Creative Commons Attribution 3.0 Unported License
*          http://creativecommons.org/licenses/by/3.0/deed.en_US
*/

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))

#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif


/**
* Returns the peak (maximum so far) resident set size (physical
* memory use) measured in bytes, or zero if the value cannot be
* determined on this OS.
*/
static size_t getPeakRSS() {
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
        return (size_t)0L;      /* Can't open? */
    if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo)) {
        close(fd);
        return (size_t)0L;      /* Can't read? */
    }
    close(fd);
    return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#else
    return (size_t) (rusage.ru_maxrss * 1024L);
#endif

#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L;          /* Unsupported. */
#endif
}


/**
* Returns the current resident set size (physical memory use) measured
* in bytes, or zero if the value cannot be determined on this OS.
*/
static size_t getCurrentRSS() {
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
        (task_info_t)&info, &infoCount) != KERN_SUCCESS)
        return (size_t)0L;      /* Can't access? */
    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE *fp = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL)
        return (size_t) 0L;      /* Can't open? */
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
        fclose(fp);
        return (size_t) 0L;      /* Can't read? */
    }
    fclose(fp);
    return (size_t) rss * (size_t) sysconf(_SC_PAGESIZE);

#else
    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L;          /* Unsupported. */
#endif
}


static void
get_gt(
    unsigned int *massQA,
    float *massQ,
    float *mass,
    size_t vecsize,
    size_t qsize,
    L2Space &l2space,
    size_t vecdim,
    vector<std::priority_queue<std::pair<float, labeltype>>> &answers,
    size_t k) {
    (vector<std::priority_queue<std::pair<float, labeltype >>>(qsize)).swap(answers);
    DISTFUNC<float> fstdistfunc_ = l2space.get_dist_func();
    cout << qsize << "\n";
    for (int i = 0; i < qsize; i++) {
        for (int j = 0; j < k; j++) {
            answers[i].emplace(0.0f, massQA[100 * i + j]);
        }
    }
}

static float
test_approx(
    float *massQ,
    size_t vecsize,
    size_t qsize,
    HierarchicalNSW<float> &appr_alg,
    size_t vecdim,
    vector<std::priority_queue<std::pair<float, labeltype>>> &answers,
    size_t k,
    void* hashed_query_buffer = nullptr,
    size_t hash_bitwidth = 0) {
    size_t correct = 0;
    size_t total = 0;
    // uncomment to test in parallel mode:
//    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < qsize; i++) {
        std::priority_queue<std::pair<float, labeltype >> result;
        if (hashed_query_buffer != nullptr) {
          result = appr_alg.searchKnn(massQ + vecdim * i, k, nullptr, hashed_query_buffer + (hash_bitwidth >> 3) * i);
        }
        else
          result = appr_alg.searchKnn(massQ + vecdim * i, k);
        std::priority_queue<std::pair<float, labeltype >> gt(answers[i]);
        unordered_set<labeltype> g;
        total += gt.size();

        while (gt.size()) {
            g.insert(gt.top().second);
            gt.pop();
        }

        while (result.size()) {
            if (g.find(result.top().second) != g.end()) {
                correct++;
            } else {
            }
            result.pop();
        }
    }
    return 1.0f * correct / total;
}

static void
test_vs_recall(
    float *massQ,
    size_t vecsize,
    size_t qsize,
    HierarchicalNSW<float> &appr_alg,
    size_t vecdim,
    vector<std::priority_queue<std::pair<float, labeltype>>> &answers,
    size_t k,
    void* hashed_query_buffer = nullptr,
    size_t hash_bitwidth = 0,
    double hash_query_time = 0.0) {
    std::cout << "Qsize: " << qsize << std::endl;
    vector<size_t> efs;  // = { 10,10,10,10,10 };
//    efs.push_back(80);
    for (int i = k; i < 30; i++) {
        efs.push_back(i);
    }
    for (int i = 30; i < 150; i += 10) {
        efs.push_back(i);
    }
//    for (int i = 150; i < 500; i += 50) {
//        efs.push_back(i);
//    }
//    for (int i = 500; i < 2000; i += 100) {
//        efs.push_back(i);
//    }
    for (size_t ef : efs) {
      double qps = std::numeric_limits<double>::min();
      float recall;
      for (int repeat = 0; repeat < 5; repeat++) {
        appr_alg.setEf(ef);
        auto s = std::chrono::high_resolution_clock::now();
//        StopW stopw = StopW();

        float tmp_recall = test_approx(massQ, vecsize, qsize, appr_alg, vecdim, answers, k, hashed_query_buffer, hash_bitwidth);
        auto e = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = e - s;
        double tmp_qps = qsize / (hash_query_time + diff.count());
//        float qps = stopw.getElapsedTimeMicro() / qsize;
        if (qps < tmp_qps) {
          recall = tmp_recall;
          qps = tmp_qps;
        }
        }

        cout << ef << "\t" << recall << "\t" << qps << " ";
        cout << std::endl;
        if (recall > 1.0) {
            cout << recall << "\t" << qps << " "; 
            cout << std::endl;
            break;
        }
    }
}

inline bool exists_test(const std::string &name) {
    ifstream f(name.c_str());
    return f.good();
}

void GenerateHashFunction(const char* file_name, L2Space* l2space, float* hash_function_buffer, uint32_t hash_bitwidth) {
  NORMFUNC<float> normfunc_ = l2space->get_norm_func(); 
  DISTFUNC<float> dotfunc_ = l2space->get_dot_func(); 
  std::normal_distribution<float> norm_dist (0.0, 1.0);
  srand(time(NULL));
  std::mt19937 gen(rand());
  void* vecdim = l2space->get_dist_func_param();
  size_t hash_len = (hash_bitwidth >> 3);
  float hash_function_norm;
  uint32_t num_superbit_blocks = (hash_bitwidth >> 6);
  uint32_t superbit_batch_size = hash_bitwidth / num_superbit_blocks;

  std::cout << "GenerateHashFunction" << std::endl;
  auto s = std::chrono::high_resolution_clock::now();
  for (size_t i = 0 ; i < hash_bitwidth; i += superbit_batch_size) {
    for (size_t dim = 0; dim < *(size_t*)vecdim; dim++) {
      hash_function_buffer[i * *(size_t*)vecdim + dim] = norm_dist(gen);
    }
    hash_function_norm = std::sqrt(normfunc_(&hash_function_buffer[i * *(size_t*)vecdim], vecdim));
    for (unsigned int dim = 0; dim < *(size_t*)vecdim; dim++) {
      hash_function_buffer[i * *(size_t*)vecdim + dim] /= hash_function_norm;
    }

    for (size_t hash_col = i + 1; hash_col < i + superbit_batch_size; hash_col++) { 
      for (size_t dim = 0; dim < *(size_t*)vecdim; dim++) {
        hash_function_buffer[hash_col * *(size_t*)vecdim + dim] = norm_dist(gen);
      }
      hash_function_norm = std::sqrt(normfunc_(&hash_function_buffer[hash_col * *(size_t*)vecdim], vecdim));
      for (unsigned int dim = 0; dim < *(size_t*)vecdim; dim++) {
        hash_function_buffer[hash_col * *(size_t*)vecdim + dim] /= hash_function_norm;
      }

      // Gram-schmidt process
      for (unsigned int compare_col = i; compare_col < hash_col; compare_col++) {
        float inner_product_between_hash = dotfunc_(&hash_function_buffer[hash_col * *(size_t*)vecdim], &hash_function_buffer[compare_col * *(size_t*)vecdim], vecdim);
        for (unsigned int dim = 0; dim < *(size_t*)vecdim; dim++) {
          hash_function_buffer[hash_col * *(size_t*)vecdim + dim] -= (inner_product_between_hash * hash_function_buffer[compare_col * *(size_t*)vecdim + dim]);
        }
      }
      hash_function_norm = std::sqrt(normfunc_(&hash_function_buffer[hash_col * *(size_t*)vecdim], vecdim));
      for (unsigned int dim = 0; dim < *(size_t*)vecdim; dim++) {
        hash_function_buffer[hash_col * *(size_t*)vecdim + dim] /= hash_function_norm;
      }
    }
  }
  auto e = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = e - s;
//    std::cout << "HashFunction generation time: " << diff.count() * 1000 << std::endl;;

  std::ofstream file_hash_function(file_name, std::ios::binary | std::ios::out);
  file_hash_function.write((char*)&hash_bitwidth, sizeof(unsigned int));
  file_hash_function.write((char*)hash_function_buffer, *(size_t*)vecdim * hash_bitwidth * sizeof(float));
  file_hash_function.close();
}

bool ReadHashFunction(const char* file_name, L2Space* l2space, float* hash_function_buffer, uint32_t hash_bitwidth) {
  std::ifstream file_hash_function(file_name, std::ios::binary);
  NORMFUNC<float> normfunc_ = l2space->get_norm_func(); 
  DISTFUNC<float> dotfunc_ = l2space->get_dot_func(); 
  void* vecdim = l2space->get_dist_func_param();
  uint64_t hash_len = (hash_bitwidth >> 3);
  if (file_hash_function.is_open()) {
    unsigned int hash_bitwidth_temp;
    file_hash_function.read((char*)&hash_bitwidth_temp, sizeof(uint32_t));
    if (hash_bitwidth != hash_bitwidth_temp) {
      file_hash_function.close();
      return false;
    }

    std::cout << "ReadHashFunction" << std::endl;
    file_hash_function.read((char*)hash_function_buffer, *(size_t*)vecdim * hash_bitwidth * sizeof(float));
    file_hash_function.close();
    return true;
  }
  else
    return false;
}

void GenerateHashedSet(const char* path_data, const char* file_name, L2Space* l2space, size_t vecsize, float* hash_function_buffer, uint32_t* hashed_set_buffer, uint32_t hash_bitwidth, float* data_buffer = NULL) {
  DISTFUNC<float> dotfunc_ = l2space->get_dot_func(); 
  void* vecdim = l2space->get_dist_func_param();
  uint64_t hash_len = (hash_bitwidth >> 3);

//  std::cout << "Loading base set to generate hashed set" << std::endl;
  float* base_set = data_buffer;
  if (base_set == NULL) {
    base_set = new float[*(size_t*)vecdim * vecsize];
    std::ifstream input(path_data, std::ios::binary);
    for (size_t i = 0; i < vecsize; i++) {
      input.seekg(4, std::ios::cur);
      input.read((char*)(base_set + i * *(size_t*)vecdim), *(size_t*)vecdim * sizeof(float));
    }
    input.close();

    std::cout << "GenerateHashedSet" << std::endl;
//    auto s = std::chrono::high_resolution_clock::now();
#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int i = 0; i < vecsize; i++) {
      void* vertex = (void*)(base_set + i * *(size_t*)vecdim);
      for (unsigned int num_integer = 0; num_integer < (hash_len >> 2); num_integer++) {
        uint32_t result = 0;

        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 0)], vecdim) > 0) << 0);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 1)], vecdim) > 0) << 1);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 2)], vecdim) > 0) << 2);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 3)], vecdim) > 0) << 3);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 4)], vecdim) > 0) << 4);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 5)], vecdim) > 0) << 5);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 6)], vecdim) > 0) << 6);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 7)], vecdim) > 0) << 7);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 8)], vecdim) > 0) << 8);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 9)], vecdim) > 0) << 9);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 10)], vecdim) > 0) << 10);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 11)], vecdim) > 0) << 11);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 12)], vecdim) > 0) << 12);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 13)], vecdim) > 0) << 13);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 14)], vecdim) > 0) << 14);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 15)], vecdim) > 0) << 15);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 16)], vecdim) > 0) << 16);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 17)], vecdim) > 0) << 17);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 18)], vecdim) > 0) << 18);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 19)], vecdim) > 0) << 19);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 20)], vecdim) > 0) << 20);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 21)], vecdim) > 0) << 21);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 22)], vecdim) > 0) << 22);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 23)], vecdim) > 0) << 23);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 24)], vecdim) > 0) << 24);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 25)], vecdim) > 0) << 25);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 26)], vecdim) > 0) << 26);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 27)], vecdim) > 0) << 27);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 28)], vecdim) > 0) << 28);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 29)], vecdim) > 0) << 29);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 30)], vecdim) > 0) << 30);
        result |= ((dotfunc_(vertex, &hash_function_buffer[*(size_t*)vecdim * (32 * num_integer + 31)], vecdim) > 0) << 31);

        hashed_set_buffer[i * (hash_len >> 2) + num_integer] = result;
      }
    }

    delete[] base_set;

    std::ofstream file_hashed_set(file_name, std::ios::binary | std::ios::out);
    file_hashed_set.write((char*)hashed_set_buffer, vecsize * hash_len);
    file_hashed_set.close();
  }
  else {
    Eigen::setNbThreads(1);
    Eigen::Map<MatrixXf> eigen_mat0(base_set, *(size_t*)vecdim, vecsize);
    Eigen::Map<MatrixXf> eigen_mat1(hash_function_buffer, *(size_t*)vecdim, hash_bitwidth);

    MatrixXf result = eigen_mat1.transpose() * eigen_mat0;

    for (size_t i = 0; i < vecsize; i++) {
      for (size_t j = 0; j < (hash_len >> 2); j++) {
        uint32_t bin_value = 0;

        bin_value |= ((result(j * 32 + 0, i) > 0) << 0);
        bin_value |= ((result(j * 32 + 1, i) > 0) << 1);
        bin_value |= ((result(j * 32 + 2, i) > 0) << 2);
        bin_value |= ((result(j * 32 + 3, i) > 0) << 3);
        bin_value |= ((result(j * 32 + 4, i) > 0) << 4);
        bin_value |= ((result(j * 32 + 5, i) > 0) << 5);
        bin_value |= ((result(j * 32 + 6, i) > 0) << 6);
        bin_value |= ((result(j * 32 + 7, i) > 0) << 7);
        bin_value |= ((result(j * 32 + 8, i) > 0) << 8);
        bin_value |= ((result(j * 32 + 9, i) > 0) << 9);
        bin_value |= ((result(j * 32 + 10, i) > 0) << 10);
        bin_value |= ((result(j * 32 + 11, i) > 0) << 11);
        bin_value |= ((result(j * 32 + 12, i) > 0) << 12);
        bin_value |= ((result(j * 32 + 13, i) > 0) << 13);
        bin_value |= ((result(j * 32 + 14, i) > 0) << 14);
        bin_value |= ((result(j * 32 + 15, i) > 0) << 15);
        bin_value |= ((result(j * 32 + 16, i) > 0) << 16);
        bin_value |= ((result(j * 32 + 17, i) > 0) << 17);
        bin_value |= ((result(j * 32 + 18, i) > 0) << 18);
        bin_value |= ((result(j * 32 + 19, i) > 0) << 19);
        bin_value |= ((result(j * 32 + 20, i) > 0) << 20);
        bin_value |= ((result(j * 32 + 21, i) > 0) << 21);
        bin_value |= ((result(j * 32 + 22, i) > 0) << 22);
        bin_value |= ((result(j * 32 + 23, i) > 0) << 23);
        bin_value |= ((result(j * 32 + 24, i) > 0) << 24);
        bin_value |= ((result(j * 32 + 25, i) > 0) << 25);
        bin_value |= ((result(j * 32 + 26, i) > 0) << 26);
        bin_value |= ((result(j * 32 + 27, i) > 0) << 27);
        bin_value |= ((result(j * 32 + 28, i) > 0) << 28);
        bin_value |= ((result(j * 32 + 29, i) > 0) << 29);
        bin_value |= ((result(j * 32 + 30, i) > 0) << 30);
        bin_value |= ((result(j * 32 + 31, i) > 0) << 31);

        hashed_set_buffer[i * (hash_len >> 2) + j] = bin_value;
      }
    }
  }

}

bool ReadHashedSet(const char* file_name, L2Space* l2space, uint32_t* hashed_set_buffer, uint32_t hash_bitwidth) {
  std::ifstream file_hashed_set(file_name, std::ios::binary);
  uint64_t hash_len = (hash_bitwidth >> 3);
  if (file_hashed_set.is_open()) {
    std::cout << "ReadHashedSet" << std::endl;
    file_hashed_set.seekg(0, std::ios::end);
    std::ios::pos_type ss = file_hashed_set.tellg();
    size_t fsize = (size_t)ss;
    file_hashed_set.seekg(0, std::ios::beg);
    file_hashed_set.read((char*)hashed_set_buffer, fsize);
    file_hashed_set.close();
    
    return true;
  }
  else
    return false;
}

void sift1M_test() {
    int efConstruction = 600;
    int M = 25;
    float tau = 0.2;
    uint64_t hash_bitwidth = 512;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    char path_hash_function[4096];
    char path_hashed_set[4096];
    char path_hashed_query[4096];
    const char *path_q = "../dataset/sift1M/sift1M_query.fvecs";
    const char *path_data = "../dataset/sift1M/sift1M_base.fvecs";
    const char *path_gt = "../dataset/sift1M/sift1M_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../dataset/hnsw_index/sift1M_ef_%d_M_%d.bin", efConstruction, M);
    snprintf(path_hash_function, sizeof(path_index), "../dataset/sift1M/sift1M_base.fvecs.hash_function_%llub", hash_bitwidth);
    snprintf(path_hashed_set, sizeof(path_index), "../dataset/sift1M/sift1M_base.fvecs.hashed_set_%llub", hash_bitwidth);
    snprintf(path_hashed_query, sizeof(path_index), "../dataset/sift1M/sift1M_query.fvecs.hashed_set_%llub", hash_bitwidth);

    cout << "Loading GT:\n";
    ifstream inputGT(path_gt, ios::binary);
		if (!inputGT.is_open()) {
			cout << "open GT error" << endl;
			exit(-1);
		}
		// Read dimension
		int numKs;
		inputGT.read((char*)&numKs, 4);
		inputGT.seekg(0, std::ios::end);
		size_t gtSize = (size_t)(inputGT.tellg());
		qsize = (gtSize / (numKs + 1) / 4);
		inputGT.seekg(0, std::ios::beg);
    unsigned int *massQA = new unsigned int[qsize * numKs];
    for (int i = 0; i < qsize; i++) {
        int t;
        inputGT.read((char *) &t, 4);
        inputGT.read((char *) (massQA + numKs * i), t * 4);
    }
    inputGT.close();

    cout << "Loading queries:\n";
    ifstream inputQ(path_q, ios::binary);
		if(!inputQ.is_open()) {
			cout << "open Q error" << endl;
			exit(-1);
		}
		inputQ.read((char*)&vecdim, 4);
		inputQ.seekg(0, std::ios::beg);
    float *massb = new float[vecdim];
    float *massQ = new float[qsize * vecdim];

    for (int i = 0; i < qsize; i++) {
        int in = 0;
        inputQ.read((char *) &in, 4);
        inputQ.read((char *) massb, in * sizeof(float));
        for (int j = 0; j < vecdim; j++) {
            massQ[i * vecdim + j] = massb[j];
        }
    }
    inputQ.close();


    float *mass = new float[vecdim];
    ifstream input(path_data, ios::binary);
		input.seekg(0, std::ios::end);
		size_t bSize = (size_t)(input.tellg());
		input.seekg(0, std::ios::beg);
		vecsize = (bSize / (vecdim + 1) / 4);
		cout << "vecdim: " << vecdim << ", bSize: " << bSize << ", vecsize: " << vecsize << endl;
    int in = 0;
    L2Space l2space(vecdim);
    
    // Sungjun Jung: Generate or Read hash function and hashed set
    float* hash_function_buffer = new float[vecdim * hash_bitwidth];
    uint32_t* hashed_set_buffer = new uint32_t[vecsize * (hash_bitwidth >> 5)];
    uint32_t* hashed_query_buffer = new uint32_t[qsize * (hash_bitwidth >> 5)];
    ifstream file_hash_function(path_hash_function, ios::binary);
    if (ReadHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth)) {
      if (ReadHashedSet(path_hashed_set, &l2space, hashed_set_buffer, hash_bitwidth)) { 
      }
      else {
        GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
      }
    }
    else {
      GenerateHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth);
      GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
    }

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, path_index, false, 0, false,
            hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, vecsize, M, efConstruction,
            100, false, hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j];
        }

        appr_alg->addPoint((void *) (massb), (size_t) 0);
        int j1 = 0;
        StopW stopw = StopW();
        StopW stopw_full = StopW();
        size_t report_every = 100000;
				cout << "vecsize: " << vecsize << endl;
#pragma omp parallel for
        for (int i = 1; i < vecsize; i++) {
            float* mass = new float[vecdim];
            int j2 = 0;
#pragma omp critical
            {
                input.read((char *) &in, 4);
                input.read((char *) massb, in * sizeof(float));
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j];
                }
                j1++;
                j2 = j1;
                if (j1 % report_every == 0) {
                    cout << j1 / (0.01 * vecsize) << " %, "
                         << report_every / (1000.0 * 1e-6 * stopw.getElapsedTimeMicro()) << " kips " << " Mem: "
                         << getCurrentRSS() / 1000000 << " Mb \n";
                    stopw.reset();
                }
            }
            appr_alg->addPoint((void *) (mass), (size_t) j2);
						delete[] mass;
        }
        input.close();
        cout << "Build time:" << 1e-6 * stopw_full.getElapsedTimeMicro() << "  seconds\n";
        appr_alg->saveIndex(path_index);
    }

    // Sungjun Jung: Hashing query at once
    GenerateHashedSet(path_q, path_hashed_query, &l2space, qsize, hash_function_buffer, hashed_query_buffer, hash_bitwidth, massQ);

    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k, hashed_query_buffer, hash_bitwidth);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    delete[] hash_function_buffer;
    delete[] hashed_set_buffer;
    delete[] hashed_query_buffer;
    return;
}

void gist1M_test() {
    int efConstruction = 800;
    int M = 35;
    float tau = 0.2;
    uint64_t hash_bitwidth = 1024;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    char path_hash_function[4096];
    char path_hashed_set[4096];
    char path_hashed_query[4096];
    const char *path_q = "../dataset/gist1M/gist1M_query.fvecs";
    const char *path_data = "../dataset/gist1M/gist1M_base.fvecs";
    const char *path_gt = "../dataset/gist1M/gist1M_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../dataset/hnsw_index/gist1M_ef_%d_M_%d.bin", efConstruction, M);
    snprintf(path_hash_function, sizeof(path_index), "../dataset/gist1M/gist1M_base.fvecs.hash_function_%llub", hash_bitwidth);
    snprintf(path_hashed_set, sizeof(path_index), "../dataset/gist1M/gist1M_base.fvecs.hashed_set_%llub", hash_bitwidth);
    snprintf(path_hashed_query, sizeof(path_index), "../dataset/gist1M/gist1M_query.fvecs.hashed_set_%llub", hash_bitwidth);

    cout << "Loading GT:\n";
    ifstream inputGT(path_gt, ios::binary);
		if (!inputGT.is_open()) {
			cout << "open GT error" << endl;
			exit(-1);
		}
		// Read dimension
		int numKs;
		inputGT.read((char*)&numKs, 4);
		inputGT.seekg(0, std::ios::end);
		size_t gtSize = (size_t)(inputGT.tellg());
		qsize = (gtSize / (numKs + 1) / 4);
		inputGT.seekg(0, std::ios::beg);
    unsigned int *massQA = new unsigned int[qsize * numKs];
    for (int i = 0; i < qsize; i++) {
        int t;
        inputGT.read((char *) &t, 4);
        inputGT.read((char *) (massQA + numKs * i), t * 4);
    }
    inputGT.close();

    cout << "Loading queries:\n";
    ifstream inputQ(path_q, ios::binary);
		if(!inputQ.is_open()) {
			cout << "open Q error" << endl;
			exit(-1);
		}
		inputQ.read((char*)&vecdim, 4);
		inputQ.seekg(0, std::ios::beg);
    float *massb = new float[vecdim];
    float *massQ = new float[qsize * vecdim];

    for (int i = 0; i < qsize; i++) {
        int in = 0;
        inputQ.read((char *) &in, 4);
        inputQ.read((char *) massb, in * sizeof(float));
        for (int j = 0; j < vecdim; j++) {
            massQ[i * vecdim + j] = massb[j];
        }
    }
    inputQ.close();


    float *mass = new float[vecdim];
    ifstream input(path_data, ios::binary);
		input.seekg(0, std::ios::end);
		size_t bSize = (size_t)(input.tellg());
		input.seekg(0, std::ios::beg);
		vecsize = (bSize / (vecdim + 1) / 4);
		cout << "vecdim: " << vecdim << ", bSize: " << bSize << ", vecsize: " << vecsize << endl;
    int in = 0;
    L2Space l2space(vecdim);
    
    // Sungjun Jung: Generate or Read hash function and hashed set
    float* hash_function_buffer = new float[vecdim * hash_bitwidth];
    uint32_t* hashed_set_buffer = new uint32_t[vecsize * (hash_bitwidth >> 5)];
    uint32_t* hashed_query_buffer = new uint32_t[qsize * (hash_bitwidth >> 5)];
    ifstream file_hash_function(path_hash_function, ios::binary);
    if (ReadHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth)) {
      if (ReadHashedSet(path_hashed_set, &l2space, hashed_set_buffer, hash_bitwidth)) { 
      }
      else {
        GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
      }
    }
    else {
      GenerateHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth);
      GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
    }

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, path_index, false, 0, false,
            hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, vecsize, M, efConstruction,
            100, false, hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j];
        }

        appr_alg->addPoint((void *) (massb), (size_t) 0);
        int j1 = 0;
        StopW stopw = StopW();
        StopW stopw_full = StopW();
        size_t report_every = 100000;
				cout << "vecsize: " << vecsize << endl;
#pragma omp parallel for
        for (int i = 1; i < vecsize; i++) {
            float* mass = new float[vecdim];
            int j2 = 0;
#pragma omp critical
            {
                input.read((char *) &in, 4);
                input.read((char *) massb, in * sizeof(float));
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j];
                }
                j1++;
                j2 = j1;
                if (j1 % report_every == 0) {
                    cout << j1 / (0.01 * vecsize) << " %, "
                         << report_every / (1000.0 * 1e-6 * stopw.getElapsedTimeMicro()) << " kips " << " Mem: "
                         << getCurrentRSS() / 1000000 << " Mb \n";
                    stopw.reset();
                }
            }
            appr_alg->addPoint((void *) (mass), (size_t) j2);
						delete[] mass;
        }
        input.close();
        cout << "Build time:" << 1e-6 * stopw_full.getElapsedTimeMicro() << "  seconds\n";
        appr_alg->saveIndex(path_index);
    }

    // Sungjun Jung: Hashing query at once
    GenerateHashedSet(path_q, path_hashed_query, &l2space, qsize, hash_function_buffer, hashed_query_buffer, hash_bitwidth, massQ);

    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k, hashed_query_buffer, hash_bitwidth);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    delete[] hash_function_buffer;
    delete[] hashed_set_buffer;
    delete[] hashed_query_buffer;
    return;
}

void crawl_test() {
    int efConstruction = 1000;
    int M = 20;
    float tau = 0.2;
    uint64_t hash_bitwidth = 512;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    char path_hash_function[4096];
    char path_hashed_set[4096];
    char path_hashed_query[4096];
    const char *path_q = "../dataset/crawl/crawl_query.fvecs";
    const char *path_data = "../dataset/crawl/crawl_base.fvecs";
    const char *path_gt = "../dataset/crawl/crawl_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../dataset/hnsw_index/crawl_ef_%d_M_%d.bin", efConstruction, M);
    snprintf(path_hash_function, sizeof(path_index), "../dataset/crawl/crawl_base.fvecs.hash_function_%llub", hash_bitwidth);
    snprintf(path_hashed_set, sizeof(path_index), "../dataset/crawl/crawl_base.fvecs.hashed_set_%llub", hash_bitwidth);
    snprintf(path_hashed_query, sizeof(path_index), "../dataset/crawl/crawl_query.fvecs.hashed_set_%llub", hash_bitwidth);


    cout << "Loading GT:\n";
    ifstream inputGT(path_gt, ios::binary);
		if (!inputGT.is_open()) {
			cout << "open GT error" << endl;
			exit(-1);
		}
		// Read dimension
		int numKs;
		inputGT.read((char*)&numKs, 4);
		inputGT.seekg(0, std::ios::end);
		size_t gtSize = (size_t)(inputGT.tellg());
		qsize = (gtSize / (numKs + 1) / 4);
		inputGT.seekg(0, std::ios::beg);
    unsigned int *massQA = new unsigned int[qsize * numKs];
    for (int i = 0; i < qsize; i++) {
        int t;
        inputGT.read((char *) &t, 4);
        inputGT.read((char *) (massQA + numKs * i), t * 4);
    }
    inputGT.close();

    cout << "Loading queries:\n";
    ifstream inputQ(path_q, ios::binary);
		if(!inputQ.is_open()) {
			cout << "open Q error" << endl;
			exit(-1);
		}
		inputQ.read((char*)&vecdim, 4);
		inputQ.seekg(0, std::ios::beg);
    float *massb = new float[vecdim];
    float *massQ = new float[qsize * vecdim];

    for (int i = 0; i < qsize; i++) {
        int in = 0;
        inputQ.read((char *) &in, 4);
        inputQ.read((char *) massb, in * sizeof(float));
        for (int j = 0; j < vecdim; j++) {
            massQ[i * vecdim + j] = massb[j];
        }
    }
    inputQ.close();


    float *mass = new float[vecdim];
    ifstream input(path_data, ios::binary);
		input.seekg(0, std::ios::end);
		size_t bSize = (size_t)(input.tellg());
		input.seekg(0, std::ios::beg);
		vecsize = (bSize / (vecdim + 1) / 4);
		cout << "vecdim: " << vecdim << ", bSize: " << bSize << ", vecsize: " << vecsize << endl;
    int in = 0;
    L2Space l2space(vecdim);
    
    // Sungjun Jung: Generate or Read hash function and hashed set
    float* hash_function_buffer = new float[vecdim * hash_bitwidth];
    uint32_t* hashed_set_buffer = new uint32_t[vecsize * (hash_bitwidth >> 5)];
    uint32_t* hashed_query_buffer = new uint32_t[qsize * (hash_bitwidth >> 5)];
    ifstream file_hash_function(path_hash_function, ios::binary);
    if (ReadHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth)) {
      if (ReadHashedSet(path_hashed_set, &l2space, hashed_set_buffer, hash_bitwidth)) { 
      }
      else {
        GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
      }
    }
    else {
      GenerateHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth);
      GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
    }

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, path_index, false, 0, false,
            hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, vecsize, M, efConstruction,
            100, false, hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j];
        }

        appr_alg->addPoint((void *) (massb), (size_t) 0);
        int j1 = 0;
        StopW stopw = StopW();
        StopW stopw_full = StopW();
        size_t report_every = 100000;
				cout << "vecsize: " << vecsize << endl;
#pragma omp parallel for
        for (int i = 1; i < vecsize; i++) {
            float* mass = new float[vecdim];
            int j2 = 0;
#pragma omp critical
            {
                input.read((char *) &in, 4);
                input.read((char *) massb, in * sizeof(float));
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j];
                }
                j1++;
                j2 = j1;
                if (j1 % report_every == 0) {
                    cout << j1 / (0.01 * vecsize) << " %, "
                         << report_every / (1000.0 * 1e-6 * stopw.getElapsedTimeMicro()) << " kips " << " Mem: "
                         << getCurrentRSS() / 1000000 << " Mb \n";
                    stopw.reset();
                }
            }
            appr_alg->addPoint((void *) (mass), (size_t) j2);
						delete[] mass;
        }
        input.close();
        cout << "Build time:" << 1e-6 * stopw_full.getElapsedTimeMicro() << "  seconds\n";
        appr_alg->saveIndex(path_index);
    }

    // Sungjun Jung: Hashing query at once
    GenerateHashedSet(path_q, path_hashed_query, &l2space, qsize, hash_function_buffer, hashed_query_buffer, hash_bitwidth, massQ);

    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k, hashed_query_buffer, hash_bitwidth);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    delete[] hash_function_buffer;
    delete[] hashed_set_buffer;
    delete[] hashed_query_buffer;
    return;
}

void glove_test() {
    int efConstruction = 2500;
    int M = 25;
    float tau = 0.2;
    uint64_t hash_bitwidth = 512;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    char path_hash_function[4096];
    char path_hashed_set[4096];
    char path_hashed_query[4096];
    const char *path_q = "../dataset/glove-100/glove-100_query.fvecs";
    const char *path_data = "../dataset/glove-100/glove-100_base.fvecs";
    const char *path_gt = "../dataset/glove-100/glove-100_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../dataset/hnsw_index/glove-100_ef_%d_M_%d.bin", efConstruction, M);
    snprintf(path_hash_function, sizeof(path_index), "../dataset/glove-100/glove-100_base.fvecs.hash_function_%llub", hash_bitwidth);
    snprintf(path_hashed_set, sizeof(path_index), "../dataset/glove-100/glove-100_base.fvecs.hashed_set_%llub", hash_bitwidth);
    snprintf(path_hashed_query, sizeof(path_index), "../dataset/glove-100/glove-100_query.fvecs.hashed_set_%llub", hash_bitwidth);

    cout << "Loading GT:\n";
    ifstream inputGT(path_gt, ios::binary);
		if (!inputGT.is_open()) {
			cout << "open GT error" << endl;
			exit(-1);
		}
		// Read dimension
		int numKs;
		inputGT.read((char*)&numKs, 4);
		inputGT.seekg(0, std::ios::end);
		size_t gtSize = (size_t)(inputGT.tellg());
		qsize = (gtSize / (numKs + 1) / 4);
		inputGT.seekg(0, std::ios::beg);
    unsigned int *massQA = new unsigned int[qsize * numKs];
    for (int i = 0; i < qsize; i++) {
        int t;
        inputGT.read((char *) &t, 4);
        inputGT.read((char *) (massQA + numKs * i), t * 4);
    }
    inputGT.close();

    cout << "Loading queries:\n";
    ifstream inputQ(path_q, ios::binary);
		if(!inputQ.is_open()) {
			cout << "open Q error" << endl;
			exit(-1);
		}
		inputQ.read((char*)&vecdim, 4);
		inputQ.seekg(0, std::ios::beg);
    float *massb = new float[vecdim];
    float *massQ = new float[qsize * vecdim];

    for (int i = 0; i < qsize; i++) {
        int in = 0;
        inputQ.read((char *) &in, 4);
        inputQ.read((char *) massb, in * sizeof(float));
        float norm = 0.0;
        for (int j = 0; j < vecdim; j++) {
            norm += massb[j] * massb[j];
        }
        norm = 1.0f / (sqrtf(norm) + 1e-30f);
        for (int j = 0; j < vecdim; j++) {
            massQ[i * vecdim + j] = massb[j] * norm;
        }
    }
    inputQ.close();


    float *mass = new float[vecdim];
    ifstream input(path_data, ios::binary);
		input.seekg(0, std::ios::end);
		size_t bSize = (size_t)(input.tellg());
		input.seekg(0, std::ios::beg);
		vecsize = (bSize / (vecdim + 1) / 4);
		cout << "vecdim: " << vecdim << ", bSize: " << bSize << ", vecsize: " << vecsize << endl;
    int in = 0;
    L2Space l2space(vecdim);
    InnerProductSpace ipspace(vecdim);
    
    // Sungjun: Generate or Read hash function and hashed set
    float* hash_function_buffer = new float[vecdim * hash_bitwidth];
    uint32_t* hashed_set_buffer = new uint32_t[vecsize * (hash_bitwidth >> 5)];
    uint32_t* hashed_query_buffer = new uint32_t[qsize * (hash_bitwidth >> 5)];
    ifstream file_hash_function(path_hash_function, ios::binary);
    if (ReadHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth)) {
      if (ReadHashedSet(path_hashed_set, &l2space, hashed_set_buffer, hash_bitwidth)) { 
      }
      else {
        GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
      }
    }
    else {
      GenerateHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth);
      GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
    }

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, path_index, false, 0, false,
            hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth, 1);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, vecsize, M, efConstruction,
            100, false, hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth, 1);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j];
        }

        appr_alg->addPoint((void *) (massb), (size_t) 0);
        int j1 = 0;
        StopW stopw = StopW();
        StopW stopw_full = StopW();
        size_t report_every = 100000;
				cout << "vecsize: " << vecsize << endl;
#pragma omp parallel for
        for (int i = 1; i < vecsize; i++) {
            float* mass = new float[vecdim];
            int j2 = 0;
#pragma omp critical
            {
                input.read((char *) &in, 4);
                input.read((char *) massb, in * sizeof(float));
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j];
                }
                j1++;
                j2 = j1;
                if (j1 % report_every == 0) {
                    cout << j1 / (0.01 * vecsize) << " %, "
                         << report_every / (1000.0 * 1e-6 * stopw.getElapsedTimeMicro()) << " kips " << " Mem: "
                         << getCurrentRSS() / 1000000 << " Mb \n";
                    stopw.reset();
                }
            }
            appr_alg->addPoint((void *) (mass), (size_t) j2);
						delete[] mass;
        }
        input.close();
        cout << "Build time:" << 1e-6 * stopw_full.getElapsedTimeMicro() << "  seconds\n";
        appr_alg->saveIndex(path_index);
    }

    // Sungjun: Hashing query at once
    GenerateHashedSet(path_q, path_hashed_query, &l2space, qsize, hash_function_buffer, hashed_query_buffer, hash_bitwidth, massQ);

    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k, hashed_query_buffer, hash_bitwidth);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    delete[] hash_function_buffer;
    delete[] hashed_set_buffer;
    delete[] hashed_query_buffer;
    return;
}

void nytimes_test() {
    int efConstruction = 500;
    int M = 16;
    float tau = 0.2;
    uint64_t hash_bitwidth = 512;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    char path_hash_function[4096];
    char path_hashed_set[4096];
    char path_hashed_query[4096];
    const char *path_q = "../dataset/nytimes/nytimes_query.fvecs";
    const char *path_data = "../dataset/nytimes/nytimes_base.fvecs";
    const char *path_gt = "../dataset/nytimes/nytimes_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../dataset/hnsw_index/nytimes_ef_%d_M_%d.bin", efConstruction, M);
    snprintf(path_hash_function, sizeof(path_index), "../dataset/nytimes/nytimes_base.fvecs.hash_function_%llub", hash_bitwidth);
    snprintf(path_hashed_set, sizeof(path_index), "../dataset/nytimes/nytimes_base.fvecs.hashed_set_%llub", hash_bitwidth);
    snprintf(path_hashed_query, sizeof(path_index), "../dataset/nytimes/nytimes_query.fvecs.hashed_set_%llub", hash_bitwidth);

    cout << "Loading GT:\n";
    ifstream inputGT(path_gt, ios::binary);
		if (!inputGT.is_open()) {
			cout << "open GT error" << endl;
			exit(-1);
		}
		// Read dimension
		int numKs;
		inputGT.read((char*)&numKs, 4);
		inputGT.seekg(0, std::ios::end);
		size_t gtSize = (size_t)(inputGT.tellg());
		qsize = (gtSize / (numKs + 1) / 4);
		inputGT.seekg(0, std::ios::beg);
    unsigned int *massQA = new unsigned int[qsize * numKs];
    for (int i = 0; i < qsize; i++) {
        int t;
        inputGT.read((char *) &t, 4);
        inputGT.read((char *) (massQA + numKs * i), t * 4);
    }
    inputGT.close();

    cout << "Loading queries:\n";
    ifstream inputQ(path_q, ios::binary);
		if(!inputQ.is_open()) {
			cout << "open Q error" << endl;
			exit(-1);
		}
		inputQ.read((char*)&vecdim, 4);
		inputQ.seekg(0, std::ios::beg);
    float *massb = new float[vecdim];
    float *massQ = new float[qsize * vecdim];

    for (int i = 0; i < qsize; i++) {
        int in = 0;
        inputQ.read((char *) &in, 4);
        inputQ.read((char *) massb, in * sizeof(float));
        float norm = 0.0;
        for (int j = 0; j < vecdim; j++) {
            norm += massb[j] * massb[j];
        }
        norm = 1.0f / (sqrtf(norm) + 1e-30f);
        for (int j = 0; j < vecdim; j++) {
            massQ[i * vecdim + j] = massb[j] * norm;
        }
    }
    inputQ.close();


    float *mass = new float[vecdim];
    ifstream input(path_data, ios::binary);
		input.seekg(0, std::ios::end);
		size_t bSize = (size_t)(input.tellg());
		input.seekg(0, std::ios::beg);
		vecsize = (bSize / (vecdim + 1) / 4);
		cout << "vecdim: " << vecdim << ", bSize: " << bSize << ", vecsize: " << vecsize << endl;
    int in = 0;
    L2Space l2space(vecdim);
    InnerProductSpace ipspace(vecdim);
    
    // Sungjun Jung: Generate or Read hash function and hashed set
    float* hash_function_buffer = new float[vecdim * hash_bitwidth];
    uint32_t* hashed_set_buffer = new uint32_t[vecsize * (hash_bitwidth >> 5)];
    uint32_t* hashed_query_buffer = new uint32_t[qsize * (hash_bitwidth >> 5)];
    ifstream file_hash_function(path_hash_function, ios::binary);
    if (ReadHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth)) {
      if (ReadHashedSet(path_hashed_set, &l2space, hashed_set_buffer, hash_bitwidth)) { 
      }
      else {
        GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
      }
    }
    else {
      GenerateHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth);
      GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
    }

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, path_index, false, 0, false,
            hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth, 1);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, vecsize, M, efConstruction,
            100, false, hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth, 1);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j];
        }

        appr_alg->addPoint((void *) (massb), (size_t) 0);
        int j1 = 0;
        StopW stopw = StopW();
        StopW stopw_full = StopW();
        size_t report_every = 100000;
				cout << "vecsize: " << vecsize << endl;
#pragma omp parallel for
        for (int i = 1; i < vecsize; i++) {
            float* mass = new float[vecdim];
            int j2 = 0;
#pragma omp critical
            {
                input.read((char *) &in, 4);
                input.read((char *) massb, in * sizeof(float));
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j];
                }
                j1++;
                j2 = j1;
                if (j1 % report_every == 0) {
                    cout << j1 / (0.01 * vecsize) << " %, "
                         << report_every / (1000.0 * 1e-6 * stopw.getElapsedTimeMicro()) << " kips " << " Mem: "
                         << getCurrentRSS() / 1000000 << " Mb \n";
                    stopw.reset();
                }
            }
            appr_alg->addPoint((void *) (mass), (size_t) j2);
						delete[] mass;
        }
        input.close();
        cout << "Build time:" << 1e-6 * stopw_full.getElapsedTimeMicro() << "  seconds\n";
        appr_alg->saveIndex(path_index);
    }

    // Sungjun Jung: Hashing query at once
    GenerateHashedSet(path_q, path_hashed_query, &l2space, qsize, hash_function_buffer, hashed_query_buffer, hash_bitwidth, massQ);

    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k, hashed_query_buffer, hash_bitwidth);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    delete[] hash_function_buffer;
    delete[] hashed_set_buffer;
    delete[] hashed_query_buffer;
    return;
}

void deep100M_test() {
    int efConstruction = 800;
    int M = 40;
    float tau = 0.2;
    uint64_t hash_bitwidth = 512;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    char path_hash_function[4096];
    char path_hashed_set[4096];
    char path_hashed_query[4096];
    const char *path_q = "../dataset/deep100M/deep100M_query.fvecs";
    const char *path_data = "../dataset/deep100M/deep100M_base.fvecs";
    const char *path_gt = "../dataset/deep100M/deep100M_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../dataset/hnsw_index/deep100M_ef_%d_M_%d.bin", efConstruction, M);
    snprintf(path_hash_function, sizeof(path_index), "../dataset/deep100M/deep100M_base.fvecs.hash_function_%llub", hash_bitwidth);
    snprintf(path_hashed_set, sizeof(path_index), "../dataset/deep100M/deep100M_base.fvecs.hashed_set_%llub", hash_bitwidth);
    snprintf(path_hashed_query, sizeof(path_index), "../dataset/deep100M/deep100M_query.fvecs.hashed_set_%llub", hash_bitwidth);


    cout << "Loading GT:\n";
    ifstream inputGT(path_gt, ios::binary);
		if (!inputGT.is_open()) {
			cout << "open GT error" << endl;
			exit(-1);
		}
		// Read dimension
		int numKs;
		inputGT.read((char*)&numKs, 4);
		inputGT.seekg(0, std::ios::end);
		size_t gtSize = (size_t)(inputGT.tellg());
		qsize = (gtSize / (numKs + 1) / 4);
		inputGT.seekg(0, std::ios::beg);
    unsigned int *massQA = new unsigned int[qsize * numKs];
    for (int i = 0; i < qsize; i++) {
        int t;
        inputGT.read((char *) &t, 4);
        inputGT.read((char *) (massQA + numKs * i), t * 4);
    }
    inputGT.close();

    cout << "Loading queries:\n";
    ifstream inputQ(path_q, ios::binary);
		if(!inputQ.is_open()) {
			cout << "open Q error" << endl;
			exit(-1);
		}
		inputQ.read((char*)&vecdim, 4);
		inputQ.seekg(0, std::ios::beg);
    float *massb = new float[vecdim];
    float *massQ = new float[qsize * vecdim];

    for (int i = 0; i < qsize; i++) {
        int in = 0;
        inputQ.read((char *) &in, 4);
        inputQ.read((char *) massb, in * sizeof(float));
        for (int j = 0; j < vecdim; j++) {
            massQ[i * vecdim + j] = massb[j];
        }
    }
    inputQ.close();


    float *mass = new float[vecdim];
    ifstream input(path_data, ios::binary);
		input.seekg(0, std::ios::end);
		size_t bSize = (size_t)(input.tellg());
		input.seekg(0, std::ios::beg);
		vecsize = (bSize / (vecdim + 1) / 4);
		cout << "vecdim: " << vecdim << ", bSize: " << bSize << ", vecsize: " << vecsize << endl;
    int in = 0;
    L2Space l2space(vecdim);
    InnerProductSpace ipspace(vecdim);
    
    // Sungjun Jung: Generate or Read hash function and hashed set
    float* hash_function_buffer = new float[vecdim * hash_bitwidth];
    uint32_t* hashed_set_buffer = new uint32_t[vecsize * (hash_bitwidth >> 5)];
    uint32_t* hashed_query_buffer = new uint32_t[qsize * (hash_bitwidth >> 5)];
    ifstream file_hash_function(path_hash_function, ios::binary);
    if (ReadHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth)) {
      if (ReadHashedSet(path_hashed_set, &l2space, hashed_set_buffer, hash_bitwidth)) { 
      }
      else {
        GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
      }
    }
    else {
      GenerateHashFunction(path_hash_function, &l2space, hash_function_buffer, hash_bitwidth);
      GenerateHashedSet(path_data, path_hashed_set, &l2space, vecsize, hash_function_buffer, hashed_set_buffer, hash_bitwidth);
    }

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, path_index, false, 0, false,
            hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, vecsize, M, efConstruction,
            100, false, hash_function_buffer, hashed_set_buffer, tau, hash_bitwidth);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j];
        }

        appr_alg->addPoint((void *) (massb), (size_t) 0);
        int j1 = 0;
        StopW stopw = StopW();
        StopW stopw_full = StopW();
        size_t report_every = 100000;
				cout << "vecsize: " << vecsize << endl;
#pragma omp parallel for
        for (int i = 1; i < vecsize; i++) {
            float* mass = new float[vecdim];
            int j2 = 0;
#pragma omp critical
            {
                input.read((char *) &in, 4);
                input.read((char *) massb, in * sizeof(float));
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j];
                }
                j1++;
                j2 = j1;
                if (j1 % report_every == 0) {
                    cout << j1 / (0.01 * vecsize) << " %, "
                         << report_every / (1000.0 * 1e-6 * stopw.getElapsedTimeMicro()) << " kips " << " Mem: "
                         << getCurrentRSS() / 1000000 << " Mb \n";
                    stopw.reset();
                }
            }
            appr_alg->addPoint((void *) (mass), (size_t) j2);
						delete[] mass;
        }
        input.close();
        cout << "Build time:" << 1e-6 * stopw_full.getElapsedTimeMicro() << "  seconds\n";
        appr_alg->saveIndex(path_index);
    }

    // Sungjun Jung: Hashing query at once
    GenerateHashedSet(path_q, path_hashed_query, &l2space, qsize, hash_function_buffer, hashed_query_buffer, hash_bitwidth, massQ);

    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k, hashed_query_buffer, hash_bitwidth);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    return;
}
