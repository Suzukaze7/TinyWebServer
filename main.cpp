#include "src/include/exception.h"
#include "src/include/header.h"
#include "src/include/http_conn.h"
#include "src/include/http_request.h"
#include "src/include/http_response.h"
#include "src/include/json.h"
#include "src/include/webserver.h"
#include <filesystem>
#include <format>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <thread>
#include <utility>

using namespace std::literals::chrono_literals;

void test_thread_pool() {
    suzukaze::ThreadPool tp;

    for (int i = 0; i < 1000; i++)
        tp.submit([] {
            static int i = 0;
            std::cout << i++ << std::endl;
            std::this_thread::sleep_for(10ms);
        });
}

void test_webserver() {
    // std::default_random_engine e(time(0));
    // std::uniform_int_distribution<> d(1025, 65535);

    suzukaze::WebServer server("0.0.0.0", 8080);
    server.get("/", [](suzukaze::HttpRequest request, suzukaze::HttpResponse response) {
        response.html("judge.html");
    });
    server.get("/file", [](suzukaze::HttpRequest request, suzukaze::HttpResponse response) {
        response.html("file.html");
    });
    server.start_server();
}

void test_json() {
    const char *json = R"({
    "1": null,
    "2": true,
    "3": 114514,
    "4": 1919.81,
    "5": "5",
    "6": [1, 2, 3],
    "7": {"inner": "inner"}
    })";
    suzukaze::json::Parser parser;
    auto val = parser.parse(json);

    // for (auto [k, v] : value.get<suzukaze::json::Object>())
    //     std::cout << k << "\n";

    std::cout << (val["1"].get<suzukaze::json::Null>() == suzukaze::json::null) << "\n";
    std::cout << val["2"].get<suzukaze::json::Bool>() << "\n";
    std::cout << val["3"].get<suzukaze::json::Int>() << "\n";
    std::cout << val["4"].get<suzukaze::json::Float>() << "\n";
    std::cout << val["5"].get<suzukaze::json::String>() << "\n";
    for (auto &x : val["6"].get<suzukaze::json::Array>())
        std::cout << x.get<suzukaze::json::Int>() << " ";
    std::cout << "\n";
    std::cout << val["7"]["inner"].get<suzukaze::json::String>() << "\n";

    suzukaze::json::Serializer serializer;

    std::cout << serializer.serialize(val);
}

int main() {
    // test_thread_pool();
    test_webserver();
    // test_json()
}