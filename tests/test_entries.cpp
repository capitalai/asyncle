#include <asyncle/meta/entries.hpp>
#include <string>
#include <vector>

namespace test_types {
struct A {
    int x;
};

struct B {
    double y;
};

struct C {
    std::string z;
};

// Test predicates
template <class T>
struct is_int: std::false_type {};

template <>
struct is_int<int>: std::true_type {};

template <class T>
struct is_string: std::false_type {};

template <>
struct is_string<std::string>: std::true_type {};

// Test meta pred with apply
struct is_arithmetic {
    template <class T>
    struct apply: std::bool_constant<std::is_arithmetic_v<T>> {};
};
}  // namespace test_types

int main() {
    using namespace asyncle;
    using namespace test_types;

    // ========== Test type_map ==========
    using map1 = type_map<int, A>;
    using map2 = type_map<std::string, B>;
    using map3 = type_map<double, C>;

    static_assert(std::same_as<map1::key, int>);
    static_assert(std::same_as<map1::mapped, A>);

    // ========== Test entry_mapped ==========
    static_assert(std::same_as<entry_mapped_t<map1>, A>);
    static_assert(std::same_as<entry_mapped_t<map2>, B>);

    // ========== Test entry_match ==========
    static_assert(entry_match<int, map1>::value);
    static_assert(!entry_match<double, map1>::value);
    static_assert(entry_match<std::string, map2>::value);

    // ========== Test first_match with type_maps ==========
    static_assert(first_match<int, map1, map2, map3>::found);
    static_assert(std::same_as<first_match<int, map1, map2, map3>::type, A>);

    static_assert(first_match<std::string, map1, map2, map3>::found);
    static_assert(std::same_as<first_match<std::string, map1, map2, map3>::type, B>);

    static_assert(!first_match<char, map1, map2, map3>::found);
    static_assert(std::same_as<first_match<char, map1, map2, map3>::type, void>);

    // ========== Test pred_map ==========
    using pred1 = pred_map<is_int, A>;
    using pred2 = pred_map<is_string, B>;

    static_assert(pred1::match<int>);
    static_assert(!pred1::match<double>);
    static_assert(pred2::match<std::string>);
    static_assert(!pred2::match<int>);

    static_assert(std::same_as<entry_mapped_t<pred1>, A>);
    static_assert(entry_match<int, pred1>::value);
    static_assert(!entry_match<double, pred1>::value);

    // ========== Test rule_map ==========
    using rule1 = rule_map<is_arithmetic, C>;
    static_assert(std::same_as<entry_mapped_t<rule1>, C>);

    // ========== Test mixed first_match ==========
    static_assert(first_match<int, map2, pred1, rule1>::found);
    static_assert(std::same_as<first_match<int, map2, pred1, rule1>::type, A>);  // matches pred1

    static_assert(first_match<float, map2, pred1, rule1>::found);
    static_assert(std::same_as<first_match<float, map2, pred1, rule1>::type, C>);  // matches rule1 (is_arithmetic)

    static_assert(first_match<std::string, map2, pred1, rule1>::found);
    static_assert(std::same_as<first_match<std::string, map2, pred1, rule1>::type, B>);  // matches map2 first

    // ========== Test legacy aliases ==========
    static_assert(std::same_as<first_match<int, map1, map2>::type, A>);
    static_assert(std::same_as<map_lookup_t<int, map1, map2>, A>);

    return 0;
}
