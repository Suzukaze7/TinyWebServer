#pragma once

#include "router.h"
#include <cstddef>
#include <string>
#include <utility>

namespace suzukaze {
static constexpr std::size_t EVENT_CNT_ = 10000;

struct Config {
    const std::size_t max_conn_cnt_;
    const std::string ip_;
    const std::uint16_t port_;
    RootRouter router_;

    Config(std::size_t max_conn_cnt, std::string &&ip, std::uint16_t port_,
           std::string &&static_dir)
        : max_conn_cnt_(max_conn_cnt), ip_(std::move(ip)), port_(port_),
          router_(RootRouter{std::move(static_dir)}) {}
};
} // namespace suzukaze