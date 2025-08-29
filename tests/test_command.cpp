#include <asyncle/base/command.hpp>
#include <string>

namespace test_types {
    struct TestError { int code; };
    struct PayloadA { int value; };
    struct PayloadB { std::string text; };
    struct PayloadC { double number; };
    
    // Test command with multiple entries
    using test_command = asyncle::command<TestError,
        asyncle::type_map<int, PayloadA>,
        asyncle::type_map<std::string, PayloadB>,
        asyncle::type_map<double, PayloadC>
    >;
    
    // Test object with command types
    struct ObjectWithCommands {
        using make_command_type = test_command;
        
        struct custom_push_command {
            using error_type = bool;
            template<class P>
            static constexpr bool accepts = true;
            template<class P>
            using payload_t = bool;
            template<class P>
            using result_t = std::expected<bool, bool>;
        };
        using push_command_type = custom_push_command;
        // take_command_type not defined, should use default
    };
    
    struct ObjectWithoutCommands {
        // No command types defined, should use defaults
    };
}

int main() {
    using namespace asyncle;
    using namespace test_types;
    
    // ========== Test basic command structure ==========
    static_assert(std::same_as<test_command::error_type, TestError>);
    
    // Test accepts
    static_assert(test_command::accepts<int>);
    static_assert(test_command::accepts<std::string>);
    static_assert(test_command::accepts<double>);
    static_assert(!test_command::accepts<float>); // not in the map
    
    // Test payload_t
    static_assert(std::same_as<test_command::payload_t<int>, PayloadA>);
    static_assert(std::same_as<test_command::payload_t<std::string>, PayloadB>);
    static_assert(std::same_as<test_command::payload_t<double>, PayloadC>);
    
    // Test result_t
    static_assert(std::same_as<test_command::result_t<int>, std::expected<PayloadA, TestError>>);
    static_assert(std::same_as<test_command::result_t<std::string>, std::expected<PayloadB, TestError>>);
    static_assert(std::same_as<test_command::result_t<float>, void>); // not accepted
    
    // ========== Test command type utilities ==========
    static_assert(std::same_as<cmd_error_t<test_command>, TestError>);
    static_assert(std::same_as<cmd_result_t<test_command, int>, std::expected<PayloadA, TestError>>);
    static_assert(cmd_accepts_v<test_command, int>);
    static_assert(!cmd_accepts_v<test_command, float>);
    
    // ========== Test is_command concept ==========
    static_assert(is_command<test_command>);
    static_assert(is_command<default_make_command>);
    static_assert(is_command<default_push_command>);
    static_assert(is_command<default_take_command>);
    
    // ========== Test command type detection ==========
    static_assert(has_make_command<ObjectWithCommands>);
    static_assert(has_push_command<ObjectWithCommands>);
    static_assert(!has_take_command<ObjectWithCommands>);
    
    static_assert(!has_make_command<ObjectWithoutCommands>);
    static_assert(!has_push_command<ObjectWithoutCommands>);
    static_assert(!has_take_command<ObjectWithoutCommands>);
    
    // ========== Test command type resolution ==========
    static_assert(std::same_as<get_make_command_t<ObjectWithCommands>, test_command>);
    static_assert(std::same_as<get_push_command_t<ObjectWithCommands>, ObjectWithCommands::custom_push_command>);
    static_assert(std::same_as<get_take_command_t<ObjectWithCommands>, default_take_command>);
    
    static_assert(std::same_as<get_make_command_t<ObjectWithoutCommands>, default_make_command>);
    static_assert(std::same_as<get_push_command_t<ObjectWithoutCommands>, default_push_command>);
    static_assert(std::same_as<get_take_command_t<ObjectWithoutCommands>, default_take_command>);
    
    // ========== Test command getter functions ==========
    ObjectWithCommands obj_with;
    ObjectWithoutCommands obj_without;
    
    // These should compile without errors
    auto make_cmd_1 = get_make_command(obj_with);
    auto push_cmd_1 = get_push_command(obj_with);
    auto take_cmd_1 = get_take_command(obj_with);
    
    auto make_cmd_2 = get_make_command(obj_without);
    auto push_cmd_2 = get_push_command(obj_without);
    auto take_cmd_2 = get_take_command(obj_without);
    
    static_assert(std::same_as<decltype(make_cmd_1), test_command>);
    static_assert(std::same_as<decltype(push_cmd_1), ObjectWithCommands::custom_push_command>);
    static_assert(std::same_as<decltype(take_cmd_1), default_take_command>);
    
    static_assert(std::same_as<decltype(make_cmd_2), default_make_command>);
    static_assert(std::same_as<decltype(push_cmd_2), default_push_command>);
    static_assert(std::same_as<decltype(take_cmd_2), default_take_command>);
    
    return 0;
}