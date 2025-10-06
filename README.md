# Asyncle

*A concept-driven, async-ready data-flow toolkit for C++23 +.*

## Vision

Provide a complete, lightweight async-ready data-flow toolkit with **value/result/object** primitives and a **try_push/try_take/work/make** pipeline so C++ developers can write composable, asynchronous data-flows **without committing to any specific thread or coroutine runtime**.

## Key Features

### Core Async Framework
✅ **Header-only library** - Zero dependencies, easy integration
✅ **C++23 concepts** - Modern type constraints and validation
✅ **Command pattern** - Flexible, extensible operation dispatch
✅ **Type mapping system** - Precise and predicate-based type dispatch
✅ **Customization Point Objects** - Standard-compliant extension mechanism
✅ **Meta-programming utilities** - Advanced type manipulation tools
✅ **Async-ready design** - Works with any execution context

### I/O Modules
✅ **Cross-platform I/O** - File, memory mapping, and process management
✅ **RAII design** - Automatic resource management with move semantics
✅ **Zero dependencies** - Direct OS primitive mapping
✅ **Structured errors** - Expected-based error handling
✅ **Comprehensive testing** - Full coverage across all modules

See **[I/O Modules Documentation](docs/IO.md)** for detailed information.

## Architecture

Asyncle provides two main components:

### 1. Async Framework (Header-only)

```
include/asyncle/
├── asyncle.hpp                    # Main header (includes all modules)
├── concept.hpp                    # Concept-only header
├── concepts/                      # Core concept definitions
│   ├── basic_concepts.hpp         # Core type concepts
│   ├── value_concepts.hpp         # Value handling concepts
│   ├── error_concepts.hpp         # Error handling concepts
│   ├── operation_concepts.hpp     # Pipeline operation concepts
│   └── utility_concepts.hpp       # Utility types and macros
├── meta/                          # Meta-programming utilities
│   ├── entries.hpp                # Type mapping and lookup systems
│   └── predicates.hpp             # Type predicate utilities
└── base/                          # Core implementation
    ├── command.hpp                # Command pattern foundation
    └── cpo.hpp                    # Customization Point Objects
```

### 2. I/O Modules (Compiled libraries)

```
include/
├── platform/                      # Platform abstraction layer
│   ├── file.hpp / file_linux.hpp
│   ├── mmap.hpp / mmap_linux.hpp
│   └── process.hpp / process_linux.hpp
└── asyncle/io/                    # RAII wrapper layer
    ├── file.hpp
    ├── mmap.hpp
    └── process.hpp
```

### Core Modules

#### Concepts Layer (`concepts/`)
Fundamental C++23 concepts that define the type requirements:

**Basic Concepts** (`basic_concepts.hpp`):
- `just_value<T, U>` - Type convertibility
- `same_type<T, U>` - Type equivalence 
- `testable<T>` - Boolean convertibility
- `object<T>` - Aggregate type checking

**Value Concepts** (`value_concepts.hpp`):
- `has_value_type<T>` - Types with `value_type` member
- `can_has_value<T>` - Types with `has_value()` method
- `can_get_value<T>` - Types with `value()` accessor

**Error Concepts** (`error_concepts.hpp`):
- `has_error_type<T>` - Types with enum `error_type`
- `can_has_error<T>` - Types with `has_error()` method
- `can_get_error<T>` - Types with `error()` accessor

**Operation Concepts** (`operation_concepts.hpp`):
- `can_push<T, Obj>` - Push operation support
- `can_take<T, Obj>` - Take operation support  
- `can_work<T, Obj>` - Work operation support
- `can_make<T, Obj, R>` - Make operation support

#### Meta Layer (`meta/`)
Advanced type manipulation and mapping utilities:

**Entries** (`entries.hpp`):
- `type_map<K,V>` - Precise type-to-type mapping
- `pred_map<P,V>` - Predicate-based type mapping
- `rule_map<R,V>` - Meta-predicate rule mapping
- `first_match<P, Entries...>` - Type lookup with fallback

**Predicates** (`predicates.hpp`):
- Common type predicates (`pred_integral`, `pred_range`, etc.)
- Rule combinators (`rule_and`, `rule_or`, `rule_not`)
- Binary concept binding (`rule_same_as`, `rule_derived_from`)
- Macro utilities (`MAKE_CONCEPT_PRED`, `MAKE_TRAIT_PRED`)

#### Base Layer (`base/`)
Core implementation providing the operational foundation:

**Command System** (`command.hpp`):
- `command<Error, Entries...>` - Command pattern with type dispatch
- Default commands (`default_make_command`, `default_push_command`, etc.)
- Command type resolution and SFINAE helpers
- `is_command<T>` concept validation

**Customization Point Objects** (`cpo.hpp`):
- `work(obj, cmd, payload)` - Core operation dispatcher
- `can_work(obj, cmd)` - Operation capability checking
- Convenience wrappers (`make`, `try_push`, `try_take`)
- `tag_invoke` customization support

## Quick Build

```bash
# Configure with tests enabled (default)
cmake -B build -DASYNCLE_BUILD_TESTS=ON

# Build the project (header-only library)
cmake --build build

# Run all tests
ctest --test-dir build

# Run specific test categories
ctest --test-dir build -R "asyncle.concepts"  # concept tests only
ctest --test-dir build -R "asyncle.meta"      # meta utilities
ctest --test-dir build -R "asyncle.base"      # base layer tests
```

### Build Options

- `ASYNCLE_BUILD_TESTS=ON/OFF` - Build unit tests (default: ON)
- Standard CMake build types: Debug, Release, RelWithDebInfo, MinSizeRel

## Testing

The project includes comprehensive unit tests for all modules:

### Async Framework Tests
- `test_basic_concepts` - Core type concept tests
- `test_value_concepts` - Value handling concept tests
- `test_error_concepts` - Error handling concept tests
- `test_operation_concepts` - Operation concept tests
- `test_utility_concepts` - Utility concept tests
- `test_entries` - Entry handling tests
- `test_predicates` - Predicate function tests
- `test_command` - Command pattern tests
- `test_cpo` - Customization Point Object tests

### I/O Module Tests
- `test_platform_file` - File I/O platform layer tests
- `test_platform_mmap` - Memory mapping platform layer tests
- `test_platform_process` - Process management platform layer tests
- `test_asyncle_io` - I/O RAII wrapper integration tests

Run specific test executables:
```bash
cd build

# Async framework tests
./test_basic_concepts
./test_value_concepts
./test_error_concepts
./test_operation_concepts
./test_utility_concepts
./test_entries
./test_predicates
./test_command
./test_cpo

# I/O module tests
./test_platform_file
./test_platform_mmap
./test_platform_process
./test_asyncle_io
```

Run tests by category:
```bash
# All async framework tests
ctest --test-dir build -R "asyncle\."

# All I/O platform tests
ctest --test-dir build -R "platform\."
```

### Test Coverage

Each test module provides comprehensive coverage:
- **Concept tests**: Validate concept definitions with positive/negative cases
- **Meta tests**: Test type mapping, lookup, and predicate systems
- **Base tests**: Verify command dispatch and CPO functionality
- **Integration tests**: End-to-end workflow validation

## Continuous Integration

Asyncle uses GitHub Actions for CI/CD with:

- **Multi-platform testing**: Ubuntu, macOS, Windows
- **Multi-compiler support**: GCC, Clang, MSVC
- **Code quality checks**: clang-format, clang-tidy
- **Code coverage**: lcov integration with Codecov

## Usage

### Basic Concepts Usage

Include concepts only for type constraints:

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

### Full System Usage

Include the complete system for advanced data-flow operations:

```cpp
#include <asyncle.hpp>
#include <expected>

// Define a simple container with command support
struct MyContainer {
    std::vector<int> data;
    
    // Define command types for this container
    using make_command_type = asyncle::command<
        std::string,  // error type
        asyncle::type_map<int, int>,           // int -> int
        asyncle::pred_map<asyncle::pred_integral, double>  // any integral -> double
    >;
    
    // Custom work implementation via member function
    template<class Cmd, class P>
    auto work(Cmd cmd, P&& payload) -> asyncle::cmd_result_t<Cmd, P> {
        if constexpr (asyncle::cmd_accepts_v<Cmd, P>) {
            if constexpr (std::same_as<std::decay_t<P>, int>) {
                data.push_back(payload);
                return payload;
            }
        }
        return std::unexpected("unsupported type");
    }
    
    // Required capability checking method
    template<class Cmd>
    bool can_work(Cmd cmd) const {
        return true; // Container can handle the command
    }
};

// Usage example
void example() {
    MyContainer container;
    
    // Use CPO to interact with the container
    auto result1 = asyncle::make(container, 42);        // returns std::expected<int, std::string>
    auto result2 = asyncle::make(container, 3.14);      // fails - double not accepted as int
    
    // Check capabilities
    if (asyncle::can_make(container)) {
        // Container supports make operations
    }
}
```

### Type Mapping Examples

```cpp
#include <asyncle/meta/entries.hpp>
#include <asyncle/meta/predicates.hpp>

// Create type mappings
using IntToString = asyncle::type_map<int, std::string>;
using FloatToDouble = asyncle::pred_map<asyncle::pred_floating_point, double>;

// Lookup types
using Result1 = asyncle::first_match<int, IntToString, FloatToDouble>;
// Result1::type is std::string, Result1::found is true

using Result2 = asyncle::first_match<float, IntToString, FloatToDouble>;  
// Result2::type is double, Result2::found is true
```

## Header Usage Patterns

**For lightweight concept-only usage:**
```cpp
#include <asyncle/concept.hpp>  // Core concepts only
```

**For meta-programming utilities:**
```cpp
#include <asyncle/meta/entries.hpp>    // Type mapping system
#include <asyncle/meta/predicates.hpp> // Type predicates
```

**For complete system:**
```cpp
#include <asyncle.hpp>  // Everything: concepts, meta, base layers
```

**Individual components:**
```cpp
#include <asyncle/base/command.hpp>  // Command pattern
#include <asyncle/base/cpo.hpp>      // Customization Point Objects
```

## Requirements

- C++23 compatible compiler
- CMake 3.23+
- Concepts support (GCC 10+, Clang 10+, MSVC 19.23+)