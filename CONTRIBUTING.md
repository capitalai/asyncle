# Contributing to Asyncle

Thank you for your interest in contributing to Asyncle! This document provides guidelines and setup instructions for contributors.

## Development Environment Setup

### Prerequisites

- C++23 compatible compiler:
  - GCC 12.0+
  - Clang 17.0+
  - MSVC 19.35+ (Visual Studio 2022 17.5+)
- CMake 3.23+
- clang-format-18 (for code formatting)

### Installing clang-format-18

**Ubuntu/Debian:**
```bash
sudo apt-get install clang-format-18
```

**macOS:**
```bash
brew install clang-format
```

**Windows:**
Download from [LLVM releases](https://releases.llvm.org/)

### Initial Setup

1. Clone the repository:
```bash
git clone https://github.com/capitalai/asyncle.git
cd asyncle
```

2. Install git hooks:
```bash
./scripts/setup-hooks.sh
```

3. Configure and build:
```bash
cmake -B build -DASYNCLE_BUILD_TESTS=ON
cmake --build build
```

4. Run tests:
```bash
ctest --test-dir build
```

## Code Style

Asyncle uses clang-format for consistent code formatting. The configuration is in `.clang-format`.

### Formatting Code

Format a single file:
```bash
clang-format-18 -i path/to/file.cpp
```

Format all staged files:
```bash
git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp|h|cc)$' | xargs clang-format-18 -i
```

Check formatting without modifying:
```bash
clang-format-18 --dry-run --Werror path/to/file.cpp
```

### Key Style Points

- **Indentation**: 4 spaces (no tabs)
- **Line length**: 120 characters
- **Naming**:
  - Classes/structs: `snake_case`
  - Functions: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Template parameters: `PascalCase`
- **Headers**: Include guards with `#ifndef ASYNCLE_MODULE_NAME_HPP`
- **Namespace**: Always use `namespace asyncle` or sub-namespaces

## Git Workflow

### Pre-commit Hooks

The project includes automatic pre-commit hooks that:
1. Check code formatting with clang-format-18
2. Run quick build checks
3. Prevent commits with formatting or build errors

If you need to bypass hooks (emergency only):
```bash
git commit --no-verify
```

### Commit Messages

Follow these conventions:

**Format:**
```
Brief summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Include motivation and context.

- Bullet points are okay
- Use present tense ("Add feature" not "Added feature")
```

**Examples:**
```
Add process module for subprocess management

Implements cross-platform process spawning with bidirectional
communication support for stdin/stdout/stderr.
```

### Branch Strategy

- `main`: Stable branch, always buildable
- Feature branches: `feature/description`
- Bug fixes: `fix/description`

## Pull Request Process

1. Ensure all tests pass locally:
   ```bash
   ctest --test-dir build
   ```

2. Verify code formatting:
   ```bash
   find include tests -name "*.hpp" -o -name "*.cpp" | xargs clang-format-18 --dry-run --Werror
   ```

3. Update documentation if needed

4. Create pull request with:
   - Clear description of changes
   - Link to related issues
   - Test results

## Testing

### Running Tests

Run all tests:
```bash
ctest --test-dir build
```

Run specific test categories:
```bash
ctest --test-dir build -R "asyncle\."     # Async framework tests
ctest --test-dir build -R "platform\."    # I/O platform tests
```

Run with verbose output:
```bash
ctest --test-dir build --output-on-failure
```

### Writing Tests

- Place tests in `tests/` directory
- Name test files: `test_<module>.cpp`
- Use descriptive test names
- Test both success and error cases
- Include edge cases

Example test structure:
```cpp
#include <asyncle/your_module.hpp>
#include <cassert>

int main() {
    // Test basic functionality
    static_assert(asyncle::some_concept<SomeType>);

    // Test runtime behavior
    assert(some_function() == expected_value);

    return 0;
}
```

## I/O Module Development

When working on I/O modules (file, mmap, process):

1. **Platform Layer** (`platform/`):
   - Thin wrappers over OS primitives
   - Use `result<T>` and `void_result` aliases
   - Keep platform-specific code in `*_linux.hpp`, `*_windows.hpp`, etc.

2. **RAII Layer** (`asyncle/io/`):
   - Automatic resource management
   - Use module-specific result types: `file_result<T>`, etc.
   - Include `error_type` and `result_type<T>` aliases in classes

3. **Testing**:
   - Platform-level tests: `test_platform_<module>.cpp`
   - Integration tests: `test_asyncle_io.cpp`

## Documentation

- Update `README.md` for user-facing changes
- Update `docs/IO.md` for I/O module changes
- Add code comments for complex logic
- Use Doxygen-style comments for public APIs

## Questions or Problems?

- Open an issue on GitHub
- Check existing issues and discussions
- Review the documentation in `docs/`

## Code of Conduct

- Be respectful and constructive
- Welcome newcomers
- Focus on the code, not the person
- Help maintain a positive community

Thank you for contributing to Asyncle!
