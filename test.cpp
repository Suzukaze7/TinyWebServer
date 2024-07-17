#include "src/include/thread_pool.hpp"
#include "src/include/webserver.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <filesystem>

using namespace std::literals::chrono_literals;

void test_thread_pool() {
    suzukaze::ThreadPool tp;

    for (int i = 0; i < 100; i++)
        tp.submit([] {
            static int i = 0;
            std::cout << i++ << std::endl;
        });

    std::this_thread::sleep_for(1s);
}

void test_http_conn() {}

void test_webserver() {
    suzukaze::WebServer server("0.0.0.0", 8080);
    server.start_server();
}

int main() {
    test_webserver();
    // test_http_conn();
}