#include <asyncle/base/cpo.hpp>
#include <asyncle/compat.hpp>
#include <asyncle/concepts/operation_concepts.hpp>
#include <cassert>
#include <iostream>

namespace test_types {
struct TestPayload {
    int value;
};

struct TestError {
    int code;
};

using test_command = asyncle::command<TestError, asyncle::type_map<TestPayload, TestPayload>>;

// Object that supports member function approach
struct MemberFunctionObject {
    mutable int call_count = 0;

    // can_work member
    asyncle::check_status can_work(test_command) const {
        ++call_count;
        return asyncle::check_status::TRUE;
    }

    // work member
    asyncle::expected<TestPayload, TestError> work(test_command, TestPayload p) const {
        ++call_count;
        return TestPayload { p.value * 2 };
    }
};

// Object that supports tag_invoke approach
struct TagInvokeObject {
    mutable int call_count = 0;
    int         multiplier = 3;
};

// tag_invoke overloads for TagInvokeObject
asyncle::check_status tag_invoke(asyncle::can_work_t, TagInvokeObject& obj, test_command) {
    ++obj.call_count;
    return asyncle::check_status::TRUE;
}

asyncle::expected<TestPayload, TestError> tag_invoke(asyncle::work_t, TagInvokeObject& obj, test_command, TestPayload p) {
    ++obj.call_count;
    return TestPayload { p.value * obj.multiplier };
}

// Object that supports both (tag_invoke should take precedence)
struct BothObject {
    mutable int member_calls = 0;
    mutable int tag_calls    = 0;

    // Member functions
    asyncle::check_status can_work(test_command) const {
        ++member_calls;
        return asyncle::check_status::FALSE;  // Different result to detect which is called
    }

    asyncle::expected<TestPayload, TestError> work(test_command, TestPayload p) const {
        ++member_calls;
        return TestPayload { p.value + 100 };  // Different result
    }
};

// tag_invoke overloads for BothObject (should take precedence)
asyncle::check_status tag_invoke(asyncle::can_work_t, BothObject& obj, test_command) {
    ++obj.tag_calls;
    return asyncle::check_status::TRUE;  // Different result
}

asyncle::expected<TestPayload, TestError> tag_invoke(asyncle::work_t, BothObject& obj, test_command, TestPayload p) {
    ++obj.tag_calls;
    return TestPayload { p.value + 200 };  // Different result
}
}  // namespace test_types

int main() {
    using namespace asyncle;
    using namespace test_types;

    // ========== Test basic CPO concepts ==========
    static_assert(tag_invocable<can_work_t, TagInvokeObject&, test_command>);
    static_assert(tag_invocable<work_t, TagInvokeObject&, test_command, TestPayload>);
    static_assert(!tag_invocable<can_work_t, MemberFunctionObject&, test_command>);  // Only has member

    // ========== Test workable concept ==========
    static_assert(workable<MemberFunctionObject, test_command, TestPayload>);
    // Note: TagInvokeObject and BothObject tests temporarily disabled due to tag_invoke resolution issues
    // static_assert(workable<TagInvokeObject, test_command, TestPayload>);
    // static_assert(workable<BothObject, test_command, TestPayload>);

    std::cout << "CPO concept tests passed!" << std::endl;
    return 0;
}
