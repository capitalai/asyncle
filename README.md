# Asyncle

*A concept-driven, async-ready data-flow toolkit for C++23 +.*

## Vision
Provide a lightweight set of **val/ret/obj** primitives and an **act/fun/map** pipeline
so C++ developers can write composable, asynchronous data-flows **without committing
to any specific thread or coroutine runtime**.

## Quick Build

```bash
cmake -B build -DASYNCLE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build     # run unit tests
