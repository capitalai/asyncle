#include <asyncle/concepts/value_concepts.hpp>
#include <optional>

struct WithValueType {
    using value_type = int;

    bool has_value() const { return true; }

    int value() const { return 42; }
};

struct WithoutValueType {
    bool has_value() const { return true; }

    int value() const { return 42; }
};

struct WithValueTypeNoHasValue {
    using value_type = int;

    int value() const { return 42; }
};

struct WithValueTypeBadHasValue {
    using value_type = int;

    void has_value() const {}

    int value() const { return 42; }
};

struct WithValueTypeNoValue {
    using value_type = int;

    bool has_value() const { return true; }
};

struct WithValueTypeBadValue {
    using value_type = int;

    bool has_value() const { return true; }

    void value() const {}
};

int main() {
    // Test has_value_type concept
    static_assert(asyncle::has_value_type<WithValueType>);
    static_assert(asyncle::has_value_type<std::optional<int>>);
    static_assert(!asyncle::has_value_type<WithoutValueType>);
    static_assert(!asyncle::has_value_type<int>);

    // Test can_has_value concept
    static_assert(asyncle::can_has_value<WithValueType>);
    static_assert(asyncle::can_has_value<std::optional<int>>);
    static_assert(!asyncle::can_has_value<WithoutValueType>);
    static_assert(!asyncle::can_has_value<WithValueTypeNoHasValue>);
    static_assert(!asyncle::can_has_value<WithValueTypeBadHasValue>);

    // Test can_get_value concept
    static_assert(asyncle::can_get_value<WithValueType>);
    static_assert(asyncle::can_get_value<std::optional<int>>);
    static_assert(!asyncle::can_get_value<WithValueTypeNoValue>);
    static_assert(!asyncle::can_get_value<WithValueTypeBadValue>);

    return 0;
}
