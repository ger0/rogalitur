#ifndef RGL_TYPES_HPP
#define RGL_TYPES_HPP

#include <cstdint>
#include <fmt/format.h>
#include <array>
#include <vector>

using u64 = uint_fast64_t;
using u32 = uint_fast32_t;
using u8  = uint_fast8_t;
using i64 = int_fast64_t;
using i32 = int_fast32_t;
using byte = unsigned char;

template <typename... T>
using Uq_ptr = std::unique_ptr<T...>;

template <typename T, size_t n>
using Arr = std::array<T, n>;

template <typename T>
using Vec = std::vector<T>;

#define DEBUG

// defer
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

#define LOG(...) \
    fmt::print(stdout, "{}\n", fmt::format(__VA_ARGS__))
#define LOG_ERR(...) \
    fmt::print(stderr, "\033[1;31m{}\033[0m\n", fmt::format(__VA_ARGS__))

#ifdef DEBUG 
#define LOG_DBG(...) \
    fmt::print(stderr, "\033[1;33m{}\033[0m\n", fmt::format(__VA_ARGS__))
#else 
#define LOG_DBG(...) ;
#endif

struct Vec2i {
    int x;
    int y;
};

struct Vec2u {
    u32 x;
    u32 y;
};

#endif // RGL_TYPES_HPP
