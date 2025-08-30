#ifndef ASYNCLE_PREDICATES_HPP
#define ASYNCLE_PREDICATES_HPP

#include "../compat/cxx23.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace asyncle {

// Macros to convert concepts/traits to predicates
#define MAKE_CONCEPT_PRED(NAME, CONCEPT) \
    template <class T>                   \
    struct NAME: std::bool_constant<CONCEPT<std::decay_t<T>>> {}

#define MAKE_TRAIT_PRED(NAME, TRAIT) \
    template <class T>               \
    struct NAME: std::bool_constant<TRAIT<std::decay_t<T>>::value> {}

// Common predicates (directly usable with pred_map)
template <class T>
struct pred_integral: std::bool_constant<std::is_integral_v<std::decay_t<T>>> {};

template <class T>
struct pred_floating_point: std::bool_constant<std::is_floating_point_v<std::decay_t<T>>> {};

template <class T>
struct pred_arithmetic: std::bool_constant<pred_integral<T>::value || pred_floating_point<T>::value> {};

MAKE_TRAIT_PRED(pred_enum, std::is_enum);
MAKE_TRAIT_PRED(pred_pointer, std::is_pointer);
MAKE_TRAIT_PRED(pred_class, std::is_class);
MAKE_TRAIT_PRED(pred_scalar, std::is_scalar);
MAKE_TRAIT_PRED(pred_trivial, std::is_trivial);
MAKE_TRAIT_PRED(pred_triv_copy, std::is_trivially_copyable);

// Range predicates - simple implementations
template <class T>
struct pred_range: std::false_type {};

template <class T>  
struct pred_range<std::vector<T>>: std::true_type {};

template <>
struct pred_range<std::string>: std::true_type {};

template <class T>
struct pred_sized_range: std::bool_constant<pred_range<T>::value> {};

template <class T>  
struct pred_contig_range: std::bool_constant<pred_range<T>::value> {};

template <class T>
struct pred_view: std::false_type {};

template <class T>
struct pred_optional: std::false_type {};

template <class U>
struct pred_optional<std::optional<U>>: std::true_type {};

template <class T>
struct pred_variant: std::false_type {};

template <class... Us>
struct pred_variant<std::variant<Us...>>: std::true_type {};

template <class T, class = void>
struct pred_tuple_like: std::false_type {};

template <class T>
struct pred_tuple_like<T, std::void_t<decltype(std::tuple_size<std::decay_t<T>>::value)>>: std::true_type {};

template <class T>
struct pred_string_view_like: std::bool_constant<std::is_constructible_v<std::string_view, std::decay_t<T>>> {};

// Rule combinators (Meta Predicates with ::template apply, for rule_map)
template <template <class> class P>
struct rule_not {
    template <class T>
    struct apply: std::bool_constant<!P<std::decay_t<T>>::value> {};
};

template <template <class> class P, template <class> class Q>
struct rule_and {
    template <class T>
    struct apply: std::bool_constant<P<std::decay_t<T>>::value && Q<std::decay_t<T>>::value> {};
};

template <template <class> class P, template <class> class Q>
struct rule_or {
    template <class T>
    struct apply: std::bool_constant<P<std::decay_t<T>>::value || Q<std::decay_t<T>>::value> {};
};

// Binary concept/condition binding
template <class U>
struct rule_same_as {
    template <class T>
    struct apply: std::bool_constant<std::same_as<std::decay_t<T>, U>> {};
};

template <class B>
struct rule_derived_from {
    template <class T>
    struct apply: std::bool_constant<std::derived_from<std::decay_t<T>, B>> {};
};

template <class U>
struct rule_convertible_to {
    template <class T>
    struct apply: std::bool_constant<std::convertible_to<std::decay_t<T>, U>> {};
};

template <class... Args>
struct rule_constructible_from {
    template <class T>
    struct apply: std::bool_constant<std::constructible_from<std::decay_t<T>, Args...>> {};
};

// Advanced binding (simplified for now)
// TODO: Fix concept template syntax for these

// Element type conditions: range's value_type also satisfies a Pred
template <template <class> class ElemPred>
struct rule_range_of {
    template <class T>
    struct apply
        : std::bool_constant<std::ranges::range<std::decay_t<T>> && ElemPred<std::ranges::range_value_t<std::decay_t<T>>>::value> {
    };
};

}  // namespace asyncle

#endif
