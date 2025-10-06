#include <asyncle/concepts/error_concepts.hpp>

enum class ErrorCode { SUCCESS, FAILURE, TIMEOUT };

struct WithErrorType {
    using error_type = ErrorCode;

    bool has_error() const { return true; }

    ErrorCode error() const { return ErrorCode::SUCCESS; }
};

struct WithoutErrorType {
    bool has_error() const { return true; }

    int error() const { return 0; }
};

struct WithNonEnumErrorType {
    using error_type = int;

    bool has_error() const { return true; }

    int error() const { return 0; }
};

struct WithErrorTypeNoHasError {
    using error_type = ErrorCode;

    ErrorCode error() const { return ErrorCode::SUCCESS; }
};

struct WithErrorTypeBadHasError {
    using error_type = ErrorCode;

    void has_error() const {}

    ErrorCode error() const { return ErrorCode::SUCCESS; }
};

struct WithErrorTypeNoError {
    using error_type = ErrorCode;

    bool has_error() const { return true; }
};

struct WithErrorTypeBadError {
    using error_type = ErrorCode;

    bool has_error() const { return true; }

    void error() const {}
};

int main() {
    // Test has_error_type concept (now accepts any type)
    static_assert(asyncle::has_error_type<WithErrorType>);
    static_assert(!asyncle::has_error_type<WithoutErrorType>);
    static_assert(asyncle::has_error_type<WithNonEnumErrorType>);  // Now passes - accepts int as error_type
    static_assert(!asyncle::has_error_type<int>);

    // Test has_enum_error_type concept (specific for enum errors)
    static_assert(asyncle::has_enum_error_type<WithErrorType>);
    static_assert(!asyncle::has_enum_error_type<WithNonEnumErrorType>);  // int is not enum

    // Test can_has_error concept
    static_assert(asyncle::can_has_error<WithErrorType>);
    static_assert(!asyncle::can_has_error<WithoutErrorType>);
    static_assert(asyncle::can_has_error<WithNonEnumErrorType>);  // Now passes - has error_type
    static_assert(!asyncle::can_has_error<WithErrorTypeNoHasError>);
    static_assert(!asyncle::can_has_error<WithErrorTypeBadHasError>);

    // Test can_get_error concept
    static_assert(asyncle::can_get_error<WithErrorType>);
    static_assert(!asyncle::can_get_error<WithErrorTypeNoError>);
    static_assert(!asyncle::can_get_error<WithErrorTypeBadError>);

    return 0;
}
