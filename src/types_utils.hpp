#ifndef RGL_TYPES_HPP
#define RGL_TYPES_HPP

#include <cstdint>
#include <fmt/format.h>
#include <array>
#include <vector>

using u64 = uint_fast64_t;
using u32 = uint_fast32_t;
using u16 = uint_fast16_t;
using u8  = uint_fast8_t;
using i64 = int_fast64_t;
//using i32 = int_fast32_t;
using i32 = int;
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

struct Vec2u {
    u16 x;
    u16 y;
    bool operator== (const Vec2u &rhs) const {
		return this->x == rhs.x && this->y == rhs.y;
    };

    // hash?
    std::size_t operator()(const Vec2u& vec) const {
        return vec.y << 16 | vec.x;
    };

    Vec2u operator / (Vec2u rhs) {
        Vec2u n_p;
        n_p.x = this->x / rhs.x;
        n_p.y = this->y / rhs.y;
        return n_p;
    }

    Vec2u operator * (Vec2u rhs) {
        Vec2u n_p;
        n_p.x = this->x * rhs.x;
        n_p.y = this->y * rhs.y;
        return n_p;
    }
};

struct Vec2i {
    i32 x;
    i32 y;
};

template <typename T>
void vec_append(Vec<T>& lhs, Vec<T>& rhs) {
	lhs.insert(
		lhs.end(),
		rhs.begin(),
		rhs.end()
	);
}

#endif // RGL_TYPES_HPP
