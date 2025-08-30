#ifndef ASYNCLE_ENTRIES_HPP
#define ASYNCLE_ENTRIES_HPP

#include <type_traits>
#include <utility>

namespace asyncle {

// Precise type mapping
template <class K, class V>
struct type_map {
    using key    = K;
    using mapped = V;
};

// Predicate-based mapping (using unary type trait: Pred<T>::value)
template <template <class> class Pred, class Payload>
struct pred_map {
    template <class T>
    static constexpr bool match = Pred<std::decay_t<T>>::value;
    using mapped                = Payload;
};

// Rule-based mapping (using Meta Pred with template<class> struct apply)
template <class MetaWithApply, class Payload>
using rule_map = pred_map<MetaWithApply::template apply, Payload>;

// Entry mapped type extraction
template <class Entry>
struct entry_mapped;

template <class K, class V>
struct entry_mapped<type_map<K, V>> {
    using type = V;
};

template <template <class> class P, class V>
struct entry_mapped<pred_map<P, V>> {
    using type = V;
};

template <class Entry>
using entry_mapped_t = typename entry_mapped<Entry>::type;

// Entry matching logic
template <class P, class Entry>
struct entry_match: std::false_type {};

template <class P, class K, class V>
struct entry_match<P, type_map<K, V>>: std::bool_constant<std::same_as<std::decay_t<P>, K>> {};

template <class P, template <class> class Pred, class V>
struct entry_match<P, pred_map<Pred, V>>: std::bool_constant<pred_map<Pred, V>::template match<P>> {};

// First-match lookup (supports type_map, pred_map, rule_map)
template <class P, class... Entries>
struct first_match;

template <class P>
struct first_match<P> {
    static constexpr bool found = false;
    using type                  = void;
};

template <class P, class E0, class... Rest>
struct first_match<P, E0, Rest...> {
    private:
    static constexpr bool m0 = entry_match<P, E0>::value;
    using tail               = first_match<P, Rest...>;

    public:
    static constexpr bool found = m0 ? true : tail::found;
    using type                  = std::conditional_t<m0, entry_mapped_t<E0>, typename tail::type>;
};

template <class K, class... Maps>
using map_lookup = first_match<K, Maps...>;

template <class K, class... Maps>
using map_lookup_t = typename first_match<K, Maps...>::type;

}  // namespace asyncle

#endif
