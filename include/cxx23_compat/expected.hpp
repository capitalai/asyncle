#ifndef CXX23_COMPAT_EXPECTED_HPP
#define CXX23_COMPAT_EXPECTED_HPP

#include <stdexcept>
#include <type_traits>
#include <utility>

// C++23 compatibility - standalone expected implementation
namespace cxx23_compat {

// Helper types for expected
struct unexpect_t { 
    explicit unexpect_t() = default; 
};
inline constexpr unexpect_t unexpect{};

// Main expected class template
template <class T, class E>
class expected {
private:
    union {
        T value_;
        E error_;
    };
    
    bool has_value_ = false;
    
public:
    using value_type = T;
    using error_type = E;
    
    // Constructors
    constexpr expected()
        requires std::is_default_constructible_v<T>
        : value_(), has_value_(true) {}
    
    constexpr expected(const T& value) : value_(value), has_value_(true) {}
    
    constexpr expected(T&& value) : value_(std::move(value)), has_value_(true) {}
    
    template <class... Args>
    constexpr expected(std::in_place_t, Args&&... args) : value_(std::forward<Args>(args)...), has_value_(true) {}
    
    constexpr expected(unexpect_t, const E& error) : error_(error), has_value_(false) {}
    
    constexpr expected(unexpect_t, E&& error) : error_(std::move(error)), has_value_(false) {}
    
    // Destructor
    constexpr ~expected() {
        if (has_value_) {
            value_.~T();
        } else {
            error_.~E();
        }
    }
    
    // Copy/move constructors and assignment operators
    constexpr expected(const expected& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new(&value_) T(other.value_);
        } else {
            new(&error_) E(other.error_);
        }
    }
    
    constexpr expected(expected&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new(&value_) T(std::move(other.value_));
        } else {
            new(&error_) E(std::move(other.error_));
        }
    }
    
    constexpr expected& operator=(const expected& other) {
        if (this != &other) {
            this->~expected();
            new(this) expected(other);
        }
        return *this;
    }
    
    constexpr expected& operator=(expected&& other) noexcept {
        if (this != &other) {
            this->~expected();
            new(this) expected(std::move(other));
        }
        return *this;
    }
    
    // Observers
    constexpr bool has_value() const noexcept { return has_value_; }
    
    constexpr explicit operator bool() const noexcept { return has_value_; }
    
    constexpr T& value() & {
        if (!has_value_) throw std::runtime_error("bad expected access");
        return value_;
    }
    
    constexpr const T& value() const& {
        if (!has_value_) throw std::runtime_error("bad expected access");
        return value_;
    }
    
    constexpr T&& value() && {
        if (!has_value_) throw std::runtime_error("bad expected access");
        return std::move(value_);
    }
    
    constexpr const T&& value() const&& {
        if (!has_value_) throw std::runtime_error("bad expected access");
        return std::move(value_);
    }
    
    constexpr E& error() & { return error_; }
    
    constexpr const E& error() const& { return error_; }
    
    constexpr E&& error() && { return std::move(error_); }
    
    constexpr const E&& error() const&& { return std::move(error_); }
    
    // Unchecked access
    constexpr T& operator*() & { return value_; }
    
    constexpr const T& operator*() const& { return value_; }
    
    constexpr T&& operator*() && { return std::move(value_); }
    
    constexpr const T&& operator*() const&& { return std::move(value_); }
    
    constexpr T* operator->() { return &value_; }
    
    constexpr const T* operator->() const { return &value_; }
};

// Specialization for expected<void, E>
template <class E>
class expected<void, E> {
private:
    union {
        E error_;
    };
    
    bool has_value_ = true;
    
public:
    using value_type = void;
    using error_type = E;
    
    // Constructors
    constexpr expected() noexcept : has_value_(true) {}
    
    constexpr expected(unexpect_t, const E& error) : error_(error), has_value_(false) {}
    
    constexpr expected(unexpect_t, E&& error) : error_(std::move(error)), has_value_(false) {}
    
    // Destructor
    constexpr ~expected() {
        if (!has_value_) {
            error_.~E();
        }
    }
    
    // Copy/move constructors and assignment operators
    constexpr expected(const expected& other) : has_value_(other.has_value_) {
        if (!has_value_) {
            new(&error_) E(other.error_);
        }
    }
    
    constexpr expected(expected&& other) noexcept : has_value_(other.has_value_) {
        if (!has_value_) {
            new(&error_) E(std::move(other.error_));
        }
    }
    
    constexpr expected& operator=(const expected& other) {
        if (this != &other) {
            this->~expected();
            new(this) expected(other);
        }
        return *this;
    }
    
    constexpr expected& operator=(expected&& other) noexcept {
        if (this != &other) {
            this->~expected();
            new(this) expected(std::move(other));
        }
        return *this;
    }
    
    // Observers
    constexpr bool has_value() const noexcept { return has_value_; }
    
    constexpr explicit operator bool() const noexcept { return has_value_; }
    
    constexpr void value() const& {
        if (!has_value_) throw std::runtime_error("bad expected access");
    }
    
    constexpr void value() && {
        if (!has_value_) throw std::runtime_error("bad expected access");
    }
    
    constexpr E& error() & { return error_; }
    
    constexpr const E& error() const& { return error_; }
    
    constexpr E&& error() && { return std::move(error_); }
    
    constexpr const E&& error() const&& { return std::move(error_); }
    
    // Void-specific operators
    constexpr void operator*() const noexcept {}
};

} // namespace cxx23_compat

#endif