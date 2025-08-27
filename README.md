# Asyncle

*A concept-driven, async-ready data-flow toolkit for C++23 +.*

## Vision
Provide a lightweight set of **value/result/object** primitives and an **try_push/try_take/work/make** pipeline
so C++ developers can write composable, asynchronous data-flows **without committing
to any specific thread or coroutine runtime**.

## Architecture

Asyncle is organized into modular concept categories:

```
include/asyncle/
├── concept.hpp                    # Main header (includes all concepts)
└── concepts/                      # Concept modules
    ├── basic_concepts.hpp         # Core type concepts
    ├── value_concepts.hpp         # Value handling concepts  
    ├── error_concepts.hpp         # Error handling concepts
    ├── operation_concepts.hpp     # Pipeline operation concepts
    └── utility_concepts.hpp       # Utility types and macros
```

### Concept Categories

#### Basic Concepts (`basic_concepts.hpp`)
- `just_value<T, U>` - Type convertibility
- `same_type<T, U>` - Type equivalence 
- `testable<T>` - Boolean convertibility
- `object<T>` - Aggregate type checking

#### Value Concepts (`value_concepts.hpp`)
- `has_value_type<T>` - Types with `value_type` member
- `can_has_value<T>` - Types with `has_value()` method
- `can_get_value<T>` - Types with `value()` accessor

#### Error Concepts (`error_concepts.hpp`)
- `has_error_type<T>` - Types with enum `error_type`
- `can_has_error<T>` - Types with `has_error()` method
- `can_get_error<T>` - Types with `error()` accessor

#### Operation Concepts (`operation_concepts.hpp`)
- `can_push<T, Obj>` - Push operation support
- `can_take<T, Obj>` - Take operation support  
- `can_work<T, Obj>` - Work operation support
- `can_make<T, Obj, R>` - Make operation support

## Quick Build

```bash
cmake -B build -DASYNCLE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build     # run unit tests
```

## Testing

The project includes comprehensive unit tests for all concept modules:

- `test_basic_concepts` - Core type concept tests
- `test_value_concepts` - Value handling concept tests
- `test_error_concepts` - Error handling concept tests
- `test_operation_concepts` - Operation concept tests
- `test_utility_concepts` - Utility concept tests

Run specific test suites:
```bash
cd build
./test_basic_concepts
./test_value_concepts
# ... etc
```

## Continuous Integration

Asyncle uses GitHub Actions for CI/CD with:

- **Multi-platform testing**: Ubuntu, macOS, Windows
- **Multi-compiler support**: GCC, Clang, MSVC
- **Code quality checks**: clang-format, clang-tidy
- **Code coverage**: lcov integration with Codecov

## Usage

Simply include the main header:

```cpp
#include <asyncle/concept.hpp>

// Use concepts to constrain your templates
template<asyncle::can_get_value T>
void process_value(const T& container) {
    if (container.has_value()) {
        auto val = container.value();
        // Process val...
    }
}
```

## Requirements

- C++23 compatible compiler
- CMake 3.23+
- Concepts support (GCC 10+, Clang 10+, MSVC 19.23+)