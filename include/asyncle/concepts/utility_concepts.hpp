#ifndef ASYNCLE_UTILITY_CONCEPTS_HPP
#define ASYNCLE_UTILITY_CONCEPTS_HPP

#include "basic_concepts.hpp"

namespace asyncle {

enum class check_status { FALSE, STABLE_FALSE, TRUE, STABLE_TRUE };

template <typename T>
concept checkable = same_type<T, check_status>;

#define ALWAYS_TRUE_CONCEPT(fun)          \
    template <typename T>                 \
    concept always_has_##fun = requires { \
        {                                 \
            []() {                        \
                constexpr T t;            \
                return t.fun();           \
            }                             \
        } -> checkable;                    \
    }

ALWAYS_TRUE_CONCEPT(value);
ALWAYS_TRUE_CONCEPT(error);
ALWAYS_TRUE_CONCEPT(can_push);
ALWAYS_TRUE_CONCEPT(can_take);
ALWAYS_TRUE_CONCEPT(can_work);
ALWAYS_TRUE_CONCEPT(can_make);

}  // namespace asyncle

#endif