#include <asyncle/concepts/operation_concepts.hpp>

struct TestObj {
    int x;
    double y;
};

enum class Status { OK, ERROR };

struct MockValue {
    using value_type = int;
    bool has_value() const { return true; }
    int value() const { return 42; }
};

struct MockResult {
    operator bool() const { return true; }
};

struct MockOperator {
    asyncle::check_status can_push() const { return asyncle::check_status::TRUE; }
    bool try_push(const TestObj&) { return true; }
    
    asyncle::check_status can_take() const { return asyncle::check_status::TRUE; }
    bool try_take(TestObj&) { return true; }
    
    asyncle::check_status can_work() const { return asyncle::check_status::TRUE; }
    bool work(TestObj&) { return true; }
    
    asyncle::check_status can_make() const { return asyncle::check_status::TRUE; }
    MockResult make(const TestObj&) { return MockResult{}; }
};

struct BadOperator {
    void can_push() const {}
    void try_push(const TestObj&) {}
};

int main() {
    // Test result concept
    static_assert(asyncle::result<bool, MockValue>);
    static_assert(asyncle::result<MockResult, MockValue>);
    static_assert(!asyncle::result<void, MockValue>);

    // Test can_push concept
    static_assert(asyncle::can_push<MockOperator, TestObj>);
    static_assert(!asyncle::can_push<BadOperator, TestObj>);
    // Note: int is not aggregate, so can_push should fail
    // static_assert(!asyncle::can_push<MockOperator, int>);

    // Test can_take concept
    static_assert(asyncle::can_take<MockOperator, TestObj>);
    static_assert(!asyncle::can_take<BadOperator, TestObj>);

    // Test can_work concept
    static_assert(asyncle::can_work<MockOperator, TestObj>);
    static_assert(!asyncle::can_work<BadOperator, TestObj>);

    // Test can_make concept
    static_assert(asyncle::can_make<MockOperator, TestObj, MockValue>);
    static_assert(!asyncle::can_make<BadOperator, TestObj, MockValue>);

    return 0;
}