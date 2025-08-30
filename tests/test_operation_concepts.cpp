#include <asyncle/concepts/operation_concepts.hpp>
#include <asyncle/compat/cxx23.hpp>
#include <cassert>
#include <iostream>

namespace test_types {
struct TestObj {
    int value = 42;
};

struct TestError {
    int code;
};

struct TestPayload {
    std::string data;
};

using test_command = asyncle::command<TestError, asyncle::type_map<TestObj, TestPayload>>;

// Object that properly implements all operations using CPO approach
struct GoodOperator {
    mutable int operation_count = 0;

    // For workable concept - supports both tag_invoke and member function fallback
    asyncle::check_status can_work(test_command) const {
        ++operation_count;
        return asyncle::check_status::TRUE;
    }

    asyncle::compat::expected<TestPayload, TestError> work(test_command, TestObj obj) const {
        ++operation_count;
        return TestPayload { "processed: " + std::to_string(obj.value) };
    }

    // For makeable/pushable/takeable - implement work with default commands
    // These will be called via CPO convenience wrappers
    TestObj work(asyncle::default_make_command, TestObj template_obj) const {
        ++operation_count;
        return TestObj { template_obj.value + 1 };
    }

    asyncle::check_status can_work(asyncle::default_make_command) const {
        ++operation_count;
        return asyncle::check_status::TRUE;
    }

    asyncle::compat::expected<bool, bool> work(asyncle::default_push_command, TestObj) const {
        ++operation_count;
        return true;
    }

    asyncle::check_status can_work(asyncle::default_push_command) const {
        ++operation_count;
        return asyncle::check_status::TRUE;
    }

    asyncle::compat::expected<bool, bool> work(asyncle::default_take_command, TestObj) const {
        ++operation_count;
        return true;
    }

    asyncle::check_status can_work(asyncle::default_take_command) const {
        ++operation_count;
        return asyncle::check_status::TRUE;
    }
};

// Object that doesn't implement operations properly
struct BadOperator {
    // Wrong return types for can_work method
    void can_work(test_command) const {}

    void can_work(asyncle::default_make_command) const {}

    void can_work(asyncle::default_push_command) const {}

    void can_work(asyncle::default_take_command) const {}

    // Missing or wrong work methods
    void work(test_command, TestObj) const {}
};

struct MockValue {
    using value_type = TestPayload;

    bool has_value() const { return true; }

    TestPayload value() const { return TestPayload { "mock" }; }
};

struct MockTestable {
    operator bool() const { return true; }
};
}  // namespace test_types

int main() {
    using namespace asyncle;
    using namespace test_types;

    // ========== Test result concept ==========
    static_assert(result<MockTestable, MockValue>);
    static_assert(result<bool, MockValue>);
    static_assert(!result<void, MockValue>);    // void is not testable
    static_assert(!result<MockTestable, int>);  // int doesn't have can_get_value

    // ========== Test workable concept ==========
    static_assert(workable<GoodOperator, test_command, TestObj>);
    static_assert(!workable<BadOperator, test_command, TestObj>);

    // Test that workable requires proper command acceptance
    using bad_command = command<TestError, type_map<float, TestPayload>>;
    static_assert(!workable<GoodOperator, bad_command, TestObj>);  // TestObj not accepted by bad_command

    // ========== Test makeable concept ==========
    static_assert(makeable<GoodOperator, TestObj>);
    static_assert(!makeable<BadOperator, TestObj>);

    // ========== Test pushable concept ==========
    static_assert(pushable<GoodOperator, TestObj>);
    static_assert(!pushable<BadOperator, TestObj>);

    // ========== Test takeable concept ==========
    static_assert(takeable<GoodOperator, TestObj>);
    static_assert(!takeable<BadOperator, TestObj>);

    // ========== Runtime tests ==========
    GoodOperator good_op;
    test_command cmd {};
    TestObj      obj { 100 };

    // Test workable operations work at runtime
    auto can_work_result = asyncle::can_work(good_op, cmd);
    assert(can_work_result == check_status::TRUE);

    auto work_result = asyncle::work(good_op, cmd, obj);
    assert(work_result.has_value());
    assert(work_result->data == "processed: 100");

    // Test makeable operations work at runtime via CPO
    auto can_make_result = asyncle::can_make(good_op);
    assert(can_make_result == check_status::TRUE);

    auto make_result = asyncle::make(good_op, obj);
    assert(make_result.value == 101);

    // Test pushable operations work at runtime via CPO
    auto can_push_result = asyncle::can_push(good_op);
    assert(can_push_result == check_status::TRUE);

    auto push_result = asyncle::try_push(good_op, obj);
    assert(push_result.has_value());
    assert(*push_result == true);

    // Test takeable operations work at runtime via CPO
    auto can_take_result = asyncle::can_take(good_op);
    assert(can_take_result == check_status::TRUE);

    auto take_result = asyncle::try_take(good_op, obj);
    assert(take_result.has_value());
    assert(*take_result == true);

    std::cout
      << "All CPO-based operation concept tests passed! Operations performed: " << good_op.operation_count << std::endl;

    return 0;
}
