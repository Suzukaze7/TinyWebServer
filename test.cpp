#include "thread_pool/thread_pool.hpp"

using namespace std::literals::chrono_literals;

void test_thread_pool() {
    suzukaze::ThreadPool tp;

    for (int i = 0; i < 100; i++)
        tp.submit(std::packaged_task([] {
            static int i = 0;
            std::cout << i++ << std::endl;
        }));

    std::this_thread::sleep_for(1s);
}

void test_http_conn() {
    
}

int main() {
    test_http_conn();
}