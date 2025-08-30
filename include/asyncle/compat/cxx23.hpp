#ifndef ASYNCLE_COMPAT_CXX23_HPP
#define ASYNCLE_COMPAT_CXX23_HPP

// C++23 compatibility header - provides missing features for older standard libraries

// Include standard headers first
#include "type_traits.hpp"
#include <stdexcept>
#include <type_traits>
#include <utility>

// Check for C++23 features and provide fallbacks
namespace asyncle::compat {

// std::expected compatibility - always use our own implementation to avoid conflicts
template <class T, class E = std::exception>
class expected {
    private:
    union {
        T value_;
        E error_;
    };

    bool has_value_;

    public:
    using value_type = T;
    using error_type = E;

    constexpr expected(const T& v): value_(v), has_value_(true) {}

    constexpr expected(T&& v): value_(std::move(v)), has_value_(true) {}

    template <class Err = E>
    constexpr expected(const Err& e)
    requires(!std::is_same_v<Err, T>)
        : error_(e), has_value_(false) {}

    template <class Err = E>
    constexpr expected(Err&& e)
    requires(!std::is_same_v<std::decay_t<Err>, T>)
        : error_(std::move(e)), has_value_(false) {}

    constexpr ~expected() {
        if(has_value_) value_.~T();
        else error_.~E();
    }

    constexpr bool has_value() const noexcept { return has_value_; }

    constexpr operator bool() const noexcept { return has_value_; }

    constexpr T& operator*() & { return value_; }

    constexpr const T& operator*() const& { return value_; }

    constexpr T&& operator*() && { return std::move(value_); }

    constexpr const T&& operator*() const&& { return std::move(value_); }

    constexpr T* operator->() { return &value_; }

    constexpr const T* operator->() const { return &value_; }

    constexpr E& error() & { return error_; }

    constexpr const E& error() const& { return error_; }

    constexpr E&& error() && { return std::move(error_); }

    constexpr const E&& error() const&& { return std::move(error_); }
};

// std::is_aggregate_v compatibility - use the implementation from type_traits.hpp
using asyncle::is_aggregate_v;

// std::same_as compatibility - always use our implementations to avoid conflicts
template <class T, class U>
concept same_as = std::is_same_v<T, U>;

template <class From, class To>
concept convertible_to = std::is_convertible_v<From, To>;

}  // namespace asyncle::compat

// Don't import into std namespace to avoid conflicts with native implementations

#endif
