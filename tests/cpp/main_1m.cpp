#include <cstdio>

void sift1M_test();
void gist1M_test();
void crawl_test();
void glove_test();
void nytimes_test();
void deep100M_test();
int main() {
    std::freopen("sift1M_search_K10.log", "w", stdout);
    sift1M_test();
    std::freopen("gist1M_search_K10.log", "w", stdout);
    gist1M_test();
    std::freopen("crawl_search_K10.log", "w", stdout);
    crawl_test();
    std::freopen("glove_search_K10.log", "w", stdout);
    glove_test();
    std::freopen("nytimes_search_K10.log", "w", stdout);
    nytimes_test();
    std::freopen("deep100M_search_K10.log", "w", stdout);
    deep100M_test();

    return 0;
}
