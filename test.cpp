#include "webserver/webserver.h"
#include "thread_pool/thread_pool.hpp"
#include <iostream>

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

void test_webserver() {
    suzukaze::WebServer server;
    server.start_server();
}

int main() {
    test_webserver();
}