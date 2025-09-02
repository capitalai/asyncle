#ifndef ASYNCLE_COMMAND_HPP
#define ASYNCLE_COMMAND_HPP

#include "../compat.hpp"
#include "../meta/entries.hpp"
#include <type_traits>

namespace asyncle {

template <class Error, class... Entries>
struct command {
    using error_type = Error;

    template <class P>
    static constexpr bool accepts = first_match<P, Entries...>::found;

    template <class P>
    using payload_t = typename first_match<P, Entries...>::type;

    template <class P>
    using result_t = std::conditional_t<accepts<P>, expected<payload_t<P>, error_type>, void>;
};

template <class Cmd>
using cmd_error_t = typename Cmd::error_type;
template <class Cmd, class P>
using cmd_result_t = typename Cmd::template result_t<P>;

template <class Cmd, class P>
inline constexpr bool cmd_accepts_v = Cmd::template accepts<P>;

template <class T>
concept is_command = requires {
    typename T::error_type;
    T::template accepts<int>;
} && requires {
    { T::template accepts<int> } -> std::convertible_to<bool>;
};

// Helper to get command types from an object type
template <class T>
concept has_make_command = requires { typename T::make_command_type; };

template <class T>
concept has_push_command = requires { typename T::push_command_type; };

template <class T>
concept has_take_command = requires { typename T::take_command_type; };

template <class T>
using make_command_t = typename T::make_command_type;

template <class T>
using push_command_t = typename T::push_command_type;

template <class T>
using take_command_t = typename T::take_command_type;

// Default commands for objects that don't define their own
struct default_make_command {
    using error_type = void;

    template <class P>
    static constexpr bool accepts = true;

    template <class P>
    using payload_t = std::remove_cvref_t<P>;

    template <class P>
    using result_t = std::conditional_t<
      accepts<P>,
      std::conditional_t<std::same_as<error_type, void>, payload_t<P>, expected<payload_t<P>, error_type>>,
      void>;
};

struct default_push_command {
    using error_type = bool;  // false = failed to push

    template <class P>
    static constexpr bool accepts = true;

    template <class P>
    using payload_t = bool;  // true = successfully pushed

    template <class P>
    using result_t = std::conditional_t<accepts<P>, expected<payload_t<P>, error_type>, void>;
};

struct default_take_command {
    using error_type = bool;  // false = failed to take

    template <class P>
    static constexpr bool accepts = true;

    template <class P>
    using payload_t = bool;  // true = successfully took

    template <class P>
    using result_t = std::conditional_t<accepts<P>, expected<payload_t<P>, error_type>, void>;
};

// Get command types for an object (either its own or default) using SFINAE
template <class T>
struct get_make_command_impl {
    using type = default_make_command;
};

template <class T>
requires has_make_command<T>
struct get_make_command_impl<T> {
    using type = make_command_t<T>;
};

template <class T>
using get_make_command_t = typename get_make_command_impl<T>::type;

template <class T>
struct get_push_command_impl {
    using type = default_push_command;
};

template <class T>
requires has_push_command<T>
struct get_push_command_impl<T> {
    using type = push_command_t<T>;
};

template <class T>
using get_push_command_t = typename get_push_command_impl<T>::type;

template <class T>
struct get_take_command_impl {
    using type = default_take_command;
};

template <class T>
requires has_take_command<T>
struct get_take_command_impl<T> {
    using type = take_command_t<T>;
};

template <class T>
using get_take_command_t = typename get_take_command_impl<T>::type;

// Convenience functions to get command instances (both const and non-const)
template <class T>
constexpr auto get_make_command(const T&) -> get_make_command_t<T> {
    return {};
}

template <class T>
constexpr auto get_make_command(T&) -> get_make_command_t<T> {
    return {};
}

template <class T>
constexpr auto get_push_command(const T&) -> get_push_command_t<T> {
    return {};
}

template <class T>
constexpr auto get_push_command(T&) -> get_push_command_t<T> {
    return {};
}

template <class T>
constexpr auto get_take_command(const T&) -> get_take_command_t<T> {
    return {};
}

template <class T>
constexpr auto get_take_command(T&) -> get_take_command_t<T> {
    return {};
}

}  // namespace asyncle

#endif
