#pragma once
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

#if __linux
#include <bits/types/struct_iovec.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#elif _WIN32
#endif

namespace suzukaze {
class MemoryBlock { // 对一块连续内存块的抽象
protected:
    void *ptr_;
    std::size_t size_;

public:
    MemoryBlock() noexcept : ptr_(), size_() {}
    MemoryBlock(void *ptr, std::size_t size) noexcept : ptr_(ptr), size_(size) {}

    void swap(MemoryBlock &oth) noexcept {
        std::swap(ptr_, oth.ptr_);
        std::swap(size_, oth.size_);
    }

    void *data() const noexcept { return ptr_; }

    std::size_t size() const noexcept { return size_; }
};

class MMap : public MemoryBlock {
public:
    MMap() = default;
    MMap(MMap &&oth) noexcept : MemoryBlock() { swap(oth); }
    MMap(std::string_view path);

    ~MMap();

    MMap &operator=(MMap &&oth) noexcept {
        MMap(std::move(oth)).swap(*this);
        return *this;
    }
};

template <std::size_t N>
class IOVec {
    friend class Descriptor;

#if __linux
    using vec = iovec;
#elif _WIN32
#endif

private:
    vec vec_[N]{};
    std::size_t idx_{};

public:
    IOVec() = default;

    void set(std::size_t idx, MemoryBlock blk) { vec_[idx] = {blk.data(), blk.size()}; }
};

class DescriptorBase {
    friend class IOMultiplex;
    friend class MMap;
    friend class Descriptor;

protected:
#if __linux
    using fd_t = int;
#elif _WIN32
#endif

protected:
    fd_t fd_;

protected:
    constexpr explicit DescriptorBase(fd_t fd) noexcept : fd_(fd) {}
    DescriptorBase() noexcept : fd_(-1) {};

public:
    DescriptorBase(DescriptorBase &&oth) noexcept : DescriptorBase() { swap(oth); }

    operator bool() const noexcept { return fd_ != -1; }
    operator std::size_t() const noexcept { return fd_; }

    friend bool operator==(const DescriptorBase &, const DescriptorBase &) = default;

    void swap(DescriptorBase &oth) noexcept { std::swap(fd_, oth.fd_); }
};

class Descriptor : public DescriptorBase {
public:
    struct OFlag {
        std::uint8_t read_ : 1;
        std::uint8_t write_ : 1;

        operator auto() {
#if __linux
            if (read_ && write_)
                return O_RDWR;
            else if (read_)
                return O_RDONLY;
            else
                return O_WRONLY;
#elif _WIN32
#endif
        }
    };

    static const DescriptorBase STDIN;

    static std::pair<Descriptor, Descriptor> pipe();

private:
    Descriptor(fd_t fd, bool no_block = true) : DescriptorBase(fd) {
        if (no_block)
            set_nonblock(fd);
    }

public:
    Descriptor() = default;
    Descriptor(Descriptor &&oth) noexcept : DescriptorBase(std::move(oth)) {}
    Descriptor(std::string_view path, OFlag flag);
    Descriptor(std::string_view ip, std::uint16_t port) : Descriptor(bind_socket(ip, port)) {}

    ~Descriptor() {
#if __linux
        if (~fd_) [[likely]]
            ::close(fd_);
#elif _WIN32
#endif
    }

    Descriptor &operator=(Descriptor &&oth) noexcept {
        Descriptor(std::move(oth)).swap(*this);
        return *this;
    }

private:
    void set_nonblock(fd_t fd);

    fd_t bind_socket(std::string_view ip, std::uint16_t port);

public:
    //@return 是否正常读完
    bool read(std::string &s);

    void write(std::string_view data);

    //@return 是否写完
    template <std::size_t N>
    bool write(IOVec<N> &vec) {
        auto &[v, idx] = vec;
#if __linux
        int cnt;
        while (idx < N && (cnt = ::writev(fd_, v + idx, N - idx)) > 0)
            while (idx < N && cnt) {
                auto &[base, len] = v[idx];
                int t = std::min<int>(cnt, len);
                base = reinterpret_cast<std::int8_t *>(base) + t;
                len -= t;
                cnt -= t;
                if (!len)
                    idx++;
            }

        return !v[N - 1].iov_len;
#elif _WIN32
#endif
    }

public:
    void listen();
    std::pair<Descriptor, std::string> accept();
};

constexpr DescriptorBase Descriptor::STDIN{0};
} // namespace suzukaze

template <>
struct std::formatter<suzukaze::Descriptor> : std::formatter<std::size_t> {
    template <class FormatContext>
    auto format(const suzukaze::Descriptor &fd, FormatContext &ctx) const {
        return std::formatter<std::size_t>::format(static_cast<std::size_t>(fd), ctx);
    }
};