#pragma once
#include<iostream>
#include<sstream>
#include<tuple>

inline std::ostream &operator<<(std::ostream &out, __int128_t a) {
    if (!a) return out << 0;
    if (a < 0) {
        out << '-';
        a = -a;
    }
    static int b[40], bn;
    for (bn = 0; a; a /= 10)
        b[++bn] = a % 10;
    for (int i = bn; i > 0; --i)
        out << b[i];
    return out;
}

#define debug(...) suzukaze::Debug(__LINE__, #__VA_ARGS__ __VA_OPT__(,) __VA_ARGS__)

namespace suzukaze {
    constexpr int OUT_LEN = 1;
    constexpr int IN_LEN = 1;
    constexpr bool SPLIT = false;

    template<typename T> constexpr bool is_string = std::is_convertible_v<T, std::string>;

    template<typename T> inline constexpr bool is_pair = false;
    template<typename T, typename U> inline constexpr bool is_pair<std::pair<T, U>> = true;

    template<typename T> inline constexpr bool is_tuple = false;
    template<typename ...T> inline constexpr bool is_tuple<std::tuple<T...>> = true;

    template<typename T, typename = std::void_t<>> inline constexpr bool is_iterable = false;
    template<typename T> inline constexpr bool is_iterable<T, std::void_t<decltype(std::begin(std::declval<T &>())), decltype(std::end(std::declval<T &>()))>> = true;

#define check(name, ...) \
    template<typename, typename = std::void_t<>> inline constexpr bool name = false;\
    template<typename T> inline constexpr bool name<T, std::void_t<__VA_ARGS__>> = true;

    check(has_pop, decltype(std::declval<T>().pop()));
    check(has_top, decltype(std::declval<T>().top()));
    check(has_front, decltype(std::declval<T>().front()));
    check(can_cout, decltype(std::cout << std::declval<T>()));
#undef check

    struct Any { template<typename T> operator T(); };
    template<std::size_t N> struct Tag : Tag<N - 1> { };
    template<> struct Tag<0> { };
    template<typename T> inline constexpr auto size_(Tag<0>) -> decltype(0u) { return 0; }
    template<typename T> inline constexpr auto size_(Tag<1>) -> decltype(T{ Any{} }, 0) { return 1; }
    template<typename T> inline constexpr auto size_(Tag<2>) -> decltype(T{ Any{}, Any{} }, 0) { return 2; }
    template<typename T> inline constexpr auto size_(Tag<3>) -> decltype(T{ Any{}, Any{}, Any{} }, 0) { return 3; }
    template<typename T> inline constexpr auto size_(Tag<4>) -> decltype(T{ Any{}, Any{}, Any{}, Any{} }, 0) { return 4; }
    template<typename T> inline constexpr auto size_(Tag<5>) -> decltype(T{ Any{}, Any{}, Any{}, Any{}, Any{} }, 0) { return 5; }
    template<typename T> inline constexpr auto size_(Tag<6>) -> decltype(T{ Any{}, Any{}, Any{}, Any{}, Any{}, Any{} }, 0) { return 6; }
    template<typename T> inline constexpr auto size_(Tag<7>) -> decltype(T{ Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{} }, 0) { return 7; }
    template<typename T> inline constexpr auto size_(Tag<8>) -> decltype(T{ Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{} }, 0) { return 8; }
    template<typename T> inline constexpr auto size() {
        if constexpr (std::is_aggregate_v<T>)
            return size_<T>(Tag<8>{});
        else
            return std::tuple_size_v<T>;
    }

    class Debug {
        const std::string in_sep = ',' + std::string(IN_LEN, ' '), out_blank = std::string(OUT_LEN, ' '),
            out_sep = out_blank + '|' + out_blank, equal = out_blank + '=' + out_blank;
        const std::string names;
        std::ostringstream sout;
        std::size_t idx = 0, cover = 0;

    public:
        template<typename T, typename ...U>
        inline Debug(const int line, std::string &&names, const T &arg, const U &...args) : names(std::move(names)) {
            sout << ".." << line << "..\t";
            if constexpr (!is_string<T> && (is_iterable<T> || std::is_pointer_v<T> || has_pop<T>) && (std::is_arithmetic_v<U> && ...))
                print(arg, false, args...);
            else
                print(arg, false), (..., print(args, true));
            std::clog << sout.str() << std::endl;
        }

        Debug(const int, const std::string &) { std::clog << "-------------------------------" << std::endl; }

    private:
        inline void print_in_sep(bool flag = true) { if (flag) sout << in_sep; }

        template<typename T, typename ...U>
        inline void print(const T &arg, const bool flag, const U &...args) {
            if constexpr (SPLIT) {
                if (flag)
                    sout << out_sep;
                while (idx < names.size() && (cover || names[idx] != ',')) {
                    cover += names[idx] == '(' || names[idx] == '<';
                    cover -= names[idx] == ')' || names[idx] == '>';
                    if (names[idx] != ' ')
                        sout << names[idx];
                    idx++;
                }
                sout << equal;
                idx++;
            } else if (!flag)
                sout << names << equal;
            else
                print_in_sep();

            print_(arg, args...);
        }

        template<typename T, typename ...U>
        inline void print_(const T &arg, const std::size_t len = -1, const U & ...args) {
            if constexpr (std::is_arithmetic_v<T>)
                sout << arg;
            else if constexpr (is_string<T>)
                sout << '"' << arg << '"';
            else if constexpr (is_pair<T>)
                sout << '(', print_(arg.first), print_in_sep(), print_(arg.second), sout << ')';
            else if constexpr (is_iterable<T>) {
                sout << '[';
                auto it = std::begin(arg);
                for (std::size_t idx = 0; idx < len && it != std::end(arg); it++, idx++)
                    print_in_sep(idx), print_(*it, args...);
                sout << ']';
            } else if constexpr (std::is_pointer_v<T>) {
                sout << '[';
                for (std::size_t idx = 0; idx < len; idx++)
                    print_in_sep(idx), print_(arg[idx], args...);
                sout << ']';
            } else if constexpr (has_pop<T>) {
                auto t = arg;
                sout << '[';
                for (std::size_t idx = 0; idx < len && t.size(); idx++)
                    if constexpr (has_top<T>)
                        print_in_sep(idx), print_(t.top(), args...), t.pop();
                    else
                        print_in_sep(idx), print_(t.front(), args...), t.pop();
                sout << ']';
            } else if constexpr (can_cout<T>)
                sout << arg;
            else if constexpr (std::is_aggregate_v<T> || is_tuple<T>) {
                if constexpr (size<T>() <= 8) {
                    sout << "{("[is_tuple<T>];
                    constexpr auto cnt = size<T>();
                    if constexpr (cnt == 1) {
                        const auto &[m1] = arg;
                        print_(m1);
                    } else if constexpr (cnt == 2) {
                        const auto &[m1, m2] = arg;
                        print_(m1), print_in_sep(), print_(m2);
                    } else if constexpr (cnt == 3) {
                        const auto &[m1, m2, m3] = arg;
                        print_(m1), print_in_sep(), print_(m2), print_in_sep(), print_(m3);
                    } else if constexpr (cnt == 4) {
                        const auto &[m1, m2, m3, m4] = arg;
                        print_(m1), print_in_sep(), print_(m2), print_in_sep(), print_(m3), print_in_sep(), print_(m4);
                    } else if constexpr (cnt == 5) {
                        const auto &[m1, m2, m3, m4, m5] = arg;
                        print_(m1), print_in_sep(), print_(m2), print_in_sep(), print_(m3), print_in_sep(), print_(m4), print_in_sep(), print_(m5);
                    } else if constexpr (cnt == 6) {
                        const auto &[m1, m2, m3, m4, m5, m6] = arg;
                        print_(m1), print_in_sep(), print_(m2), print_in_sep(), print_(m3), print_in_sep(), print_(m4), print_in_sep(), print_(m5), print_in_sep(), print_(m6);
                    } else if constexpr (cnt == 7) {
                        const auto &[m1, m2, m3, m4, m5, m6, m7] = arg;
                        print_(m1), print_in_sep(), print_(m2), print_in_sep(), print_(m3), print_in_sep(), print_(m4), print_in_sep(), print_(m5), print_in_sep(), print_(m6), print_in_sep(), print_(m7);
                    } else {
                        const auto &[m1, m2, m3, m4, m5, m6, m7, m8] = arg;
                        print_(m1), print_in_sep(), print_(m2), print_in_sep(), print_(m3), print_in_sep(), print_(m4), print_in_sep(), print_(m5), print_in_sep(), print_(m6), print_in_sep(), print_(m7), print_in_sep(), print_(m8);
                    }
                    sout << "})"[is_tuple<T>];
                }
            } else
                sout << "NoDebugType";
        }
    };
}