#include <asyncle/concepts/basic_concepts.hpp>

struct TestStruct {
    int x;
    double y;
};

class TestClass {
public:
    int value;
};

int main() {
    // Test just_value concept
    static_assert(asyncle::just_value<int>);
    static_assert(asyncle::just_value<int, int>);
    static_assert(asyncle::just_value<double, int>);
    static_assert(asyncle::just_value<bool, int>);
    static_assert(!asyncle::just_value<TestStruct, int>);

    // Test same_type concept
    static_assert(asyncle::same_type<int, int>);
    static_assert(asyncle::same_type<const int, int>);
    static_assert(asyncle::same_type<int&, int>);
    static_assert(asyncle::same_type<const int&, int>);
    static_assert(!asyncle::same_type<int, double>);
    static_assert(!asyncle::same_type<int, TestStruct>);

    // Test testable concept
    static_assert(asyncle::testable<bool>);
    static_assert(asyncle::testable<int>);
    static_assert(asyncle::testable<double>);
    static_assert(!asyncle::testable<TestStruct>);

    // Test object concept
    static_assert(asyncle::object<TestStruct>);
    static_assert(asyncle::object<TestClass>); // TestClass is actually aggregate
    static_assert(!asyncle::object<int>);

    return 0;
}