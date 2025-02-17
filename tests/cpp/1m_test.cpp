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
    size_t k) {
    size_t correct = 0;
    size_t total = 0;
    // uncomment to test in parallel mode:
    //#pragma omp parallel for
    for (int i = 0; i < qsize; i++) {
        std::priority_queue<std::pair<float, labeltype >> result = appr_alg.searchKnn(massQ + vecdim * i, k);
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
    size_t k) {
    vector<size_t> efs;  // = { 10,10,10,10,10 };
    efs.push_back(73);
//    for (int i = ik; i < 30; i++) {
//        efs.push_back(i);
//    }
//    for (int i = 30; i < 150; i += 10) {
//        efs.push_back(i);
//    }
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
#ifdef PROFILE
        appr_alg.profile_time.clear();
        appr_alg.profile_time.resize(7, 0.0);
#endif
        auto s = std::chrono::high_resolution_clock::now();
//        StopW stopw = StopW();

//        // [ARC-SJ] Initialize visited lists
//        appr_alg.visited_list_pool_.reset();
//        appr_alg.visited_list_pool_ = std::unique_ptr<VisitedListPool>(new VisitedListPool(1, appr_alg.max_elements_));

        float tmp_recall = test_approx(massQ, vecsize, qsize, appr_alg, vecdim, answers, k);
        auto e = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = e - s;
        double tmp_qps = qsize/ diff.count();
        float time_us_per_query = diff.count() * 1000000 / qsize;
//        float time_us_per_query = stopw.getElapsedTimeMicro() / qsize;
        if (qps < tmp_qps) {
          recall = tmp_recall;
          qps = tmp_qps;
        }
        }

        cout << ef << "\t" << recall << "\t" << qps << " ";
#ifdef PROFILE
        cout << appr_alg.profile_time[0] * 1000000 / qsize << " " << appr_alg.profile_time[1] * 1000000 / qsize << " " << appr_alg.profile_time[2] * 1000000 / qsize << " " << appr_alg.profile_time[3] * 1000000 / qsize << " ";
#endif
#ifdef GET_EFFICIENCY
        cout << appr_alg.total_similarity_compute << " " << appr_alg.effective_similarity_compute << " " << 100.0 * appr_alg.effective_similarity_compute / appr_alg.total_similarity_compute << " ";
#endif
        cout << std::endl;
        if (recall > 1.0) {
            cout << recall << "\t" << qps << " ";
#ifdef PROFILE
            cout << appr_alg.profile_time[0] * 1000000 / qsize << " " << appr_alg.profile_time[1] * 1000000 / qsize << " " << appr_alg.profile_time[2] * 1000000 / qsize << " ";
#endif
#ifdef GET_EFFICIENCY
            cout << appr_alg.total_similarity_compute << " " << appr_alg.effective_similarity_compute << " " <<  100.0 * appr_alg.effective_similarity_compute / appr_alg.total_similarity_compute << " ";
#endif
            cout << std::endl;
            break;
        }
    }
}

inline bool exists_test(const std::string &name) {
    ifstream f(name.c_str());
    return f.good();
}


void sift1M_test() {
    int efConstruction = 600;
    int M = 25;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    const char *path_q = "../../../dataset/sift1M/sift1M_query.fvecs";
    const char *path_data = "../../../dataset/sift1M/sift1M_base.fvecs";
    const char *path_gt = "../../../dataset/sift1M/sift1M_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../../../dataset/hnsw_index/sift1M_ef_%d_M_%d.bin", efConstruction, M);


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

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, path_index, false);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, vecsize, M, efConstruction);

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


    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    return;
}

void gist1M_test() {
    int efConstruction = 800;
    int M = 35;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    const char *path_q = "../../../dataset/gist1M/gist1M_query.fvecs";
    const char *path_data = "../../../dataset/gist1M/gist1M_base.fvecs";
    const char *path_gt = "../../../dataset/gist1M/gist1M_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../../../dataset/hnsw_index/gist1M_ef_%d_M_%d.bin", efConstruction, M);


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

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, path_index, false);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, vecsize, M, efConstruction);

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


    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    return;
}

void crawl_test() {
    int efConstruction = 1000;
    int M = 20;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    const char *path_q = "../../../dataset/crawl/crawl_query.fvecs";
    const char *path_data = "../../../dataset/crawl/crawl_base.fvecs";
    const char *path_gt = "../../../dataset/crawl/crawl_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../../../dataset/hnsw_index/crawl_ef_%d_M_%d.bin", efConstruction, M);


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

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, path_index, false);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, vecsize, M, efConstruction);

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


    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    return;
}

void glove_test() {
    int efConstruction = 2500;
    int M = 25;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    const char *path_q = "../../../dataset/glove-100/glove-100_query.fvecs";
    const char *path_data = "../../../dataset/glove-100/glove-100_base.fvecs";
    const char *path_gt = "../../../dataset/glove-100/glove-100_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../../../dataset/hnsw_index/glove-100_ef_%d_M_%d.bin", efConstruction, M);


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
    InnerProductSpace ipspace(vecdim);
    L2Space l2space(vecdim);

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, path_index, false);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, vecsize, M, efConstruction);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        float norm = 0.0;
        for (int j = 0; j < vecdim; j++) {
            norm += massb[j] * massb[j];
        }
        norm = 1.0f / (sqrtf(norm) + 1e-30f);
        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j] * norm;
        }

        appr_alg->addPoint((void *) (mass), (size_t) 0);
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
                float norm_0 = 0.0;
                for (int j = 0; j < vecdim; j++) {
                    norm_0 += massb[j] * massb[j];
                }
                norm_0 = 1.0f / (sqrtf(norm_0) + 1e-30f);
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j] * norm_0;
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


    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";

    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    return;
}

void nytimes_test() {
    int efConstruction = 500;
    int M = 16;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    const char *path_q = "../../../dataset/nytimes/nytimes_query.fvecs";
    const char *path_data = "../../../dataset/nytimes/nytimes_base.fvecs";
    const char *path_gt = "../../../dataset/nytimes/nytimes_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../../../dataset/hnsw_index/nytimes_ef_%d_M_%d.bin", efConstruction, M);


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
    InnerProductSpace ipspace(vecdim);
    L2Space l2space(vecdim);

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, path_index, false);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&ipspace, vecsize, M, efConstruction);

        input.read((char *) &in, 4);
        input.read((char *) massb, in * sizeof(float));

        float norm = 0.0;
        for (int j = 0; j < vecdim; j++) {
            norm += massb[j] * massb[j];
        }
        norm = 1.0f / (sqrtf(norm));
        for (int j = 0; j < vecdim; j++) {
            mass[j] = massb[j] * norm;
        }

        appr_alg->addPoint((void *) (mass), (size_t) 0);
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
                float norm_0 = 0.0;
                for (int j = 0; j < vecdim; j++) {
                    norm_0 += massb[j] * massb[j];
                }
                norm_0 = 1.0f / (sqrtf(norm_0) + 1e-30f);
                for (int j = 0; j < vecdim; j++) {
                    mass[j] = massb[j] * norm_0;
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


    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";

    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    return;
}

void deep100M_test() {
    int efConstruction = 800;
    int M = 40;

    size_t vecsize = 1000000;

    size_t qsize = 10000;
    size_t vecdim = 128;
    char path_index[4096];
    const char *path_q = "../../../dataset/deep100M/deep100M_query.fvecs";
    const char *path_data = "../../../dataset/deep100M/deep100M_base.fvecs";
    const char *path_gt = "../../../dataset/deep100M/deep100M_groundtruth.ivecs";
    snprintf(path_index, sizeof(path_index), "../../../dataset/hnsw_index/deep100M_ef_%d_M_%d.bin", efConstruction, M);


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

    HierarchicalNSW<float> *appr_alg;
    if (exists_test(path_index)) {
        cout << "Loading index from " << path_index << ":\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, path_index, false);
        cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    } else {
        cout << "Building index:\n";
        appr_alg = new HierarchicalNSW<float>(&l2space, vecsize, M, efConstruction);

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


    vector<std::priority_queue<std::pair<float, labeltype >>> answers;
    size_t k = 10;
    cout << "Parsing gt:\n";
    get_gt(massQA, massQ, mass, vecsize, qsize, l2space, vecdim, answers, k);
    cout << "Loaded gt\n";
    for (int i = 0; i < 1; i++)
        test_vs_recall(massQ, vecsize, qsize, *appr_alg, vecdim, answers, k);
    cout << "Actual memory usage: " << getCurrentRSS() / 1000000 << " Mb \n";
    return;
}
