#ifndef ASYNCLE_COMPAT_RANGES_HPP
#define ASYNCLE_COMPAT_RANGES_HPP

#include <iterator>
#include <type_traits>
#include "../compat/concepts.hpp"

namespace asyncle {

// Basic ranges concepts compatibility
#ifdef __cpp_lib_ranges
#include <ranges>
namespace ranges = std::ranges;
#else
// Basic ranges implementation
namespace ranges {
    template <class T>
    concept range = requires(T& t) {
        std::begin(t);
        std::end(t);
    };

    template <class T>
    concept sized_range = range<T> && requires(T& t) {
        { std::size(t) } -> convertible_to<std::size_t>;
    };

    template <class T>
    concept contiguous_range = range<T> && requires(T& t) {
        { std::data(t) } -> convertible_to<const std::remove_reference_t<decltype(*std::begin(t))>*>;
    };

    template <class T>
    concept view = range<T> && std::is_move_constructible_v<T> && std::is_default_constructible_v<T>;

    template <class R>
    using range_value_t = std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>;
}

// Import into std namespace
namespace std::ranges {
    using namespace asyncle::ranges;
}
#endif

} // namespace asyncle

#endif