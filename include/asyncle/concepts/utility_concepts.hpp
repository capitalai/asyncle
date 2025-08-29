#ifndef ASYNCLE_UTILITY_CONCEPTS_HPP
#define ASYNCLE_UTILITY_CONCEPTS_HPP

#include "basic_concepts.hpp"
#include "../base/cpo.hpp"

namespace asyncle {

enum class check_status { FALSE, STABLE_FALSE, TRUE, STABLE_TRUE };

template <typename T>
concept checkable = same_type<T, check_status>;

#define ALWAYS_HAS_CONCEPT(fun)           \
    template <typename T>                 \
    concept always_has_##fun = requires { \
        {                                 \
            []() {                        \
                constexpr T t;            \
                return t.has_##fun();     \
            }()                           \
        } -> checkable;                   \
    }

#define ALWAYS_CAN_CONCEPT(fun)           \
    template <typename T>                 \
    concept always_can_##fun = requires { \
        {                                 \
            []() {                        \
                constexpr T t;            \
                return can_##fun(t);      \
            }()                           \
        } -> checkable;                   \
    }

ALWAYS_HAS_CONCEPT(value);
ALWAYS_HAS_CONCEPT(error);
ALWAYS_CAN_CONCEPT(push);
ALWAYS_CAN_CONCEPT(take);
ALWAYS_CAN_CONCEPT(work);
ALWAYS_CAN_CONCEPT(make);

}  // namespace asyncle

#endif
