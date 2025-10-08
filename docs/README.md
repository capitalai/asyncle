# asyncle Documentation Index

## Quick Start

### Format Layer (JSON & Serialization)

📖 **[FORMAT.md](FORMAT.md)** - Complete guide to the format layer

Quick example:
```cpp
#include <asyncle/format/json.hpp>
#include <asyncle/format/serialize.hpp>

// JSON Parsing
auto parser = asyncle::format::json::make_parser()
    .source(json_string)
    .make();
auto doc = parser.parse();

// Serialization
struct Config { int port; std::string host; };
auto json = asyncle::format::to_json(config);
auto config = asyncle::format::from_json<Config>(json);
```

### I/O and Hardware

- [IO.md](IO.md) - I/O module design
- [HARDWARE_MODULE_DESIGN.md](HARDWARE_MODULE_DESIGN.md) - Hardware abstraction layer

---

## Architecture Decision Records (ADR)

📋 Design decisions and rationale

- [ADR-001: Format Layer Zero-Coupling Design](adr/001-format-layer-zero-coupling.md) ⭐
  - **Status**: Accepted
  - **Date**: 2025-10-08
  - Complete isolation between asyncle and format implementations

---

## Reference Documentation

### Format Layer

- **[FORMAT.md](FORMAT.md)** ⭐ - Complete guide (architecture, usage, customization)
- [EXTERNAL_DEPENDENCIES.md](EXTERNAL_DEPENDENCIES.md) - Installing simdjson/Glaze

### Archive

- [archive/](archive/) - Historical documentation (superseded designs, development notes)

---

## Project Structure

```
asyncle/
├── docs/
│   ├── README.md                  # This file
│   ├── FORMAT.md                  # Format layer complete guide ⭐
│   ├── EXTERNAL_DEPENDENCIES.md   # Dependency installation
│   ├── adr/                       # Architecture Decision Records
│   └── archive/                   # Historical documents
│
├── include/
│   ├── format/                    # Foundation Layer (sync, pluggable)
│   │   ├── json/
│   │   │   ├── parser.hpp         # Type alias (key!) ⭐
│   │   │   └── simdjson.hpp       # simdjson adapter
│   │   └── serialize/
│   │       └── glaze.hpp          # Glaze adapter
│   │
│   └── asyncle/format/            # Integration Layer (zero-coupled) ⭐
│       ├── json.hpp               # Recommended entry point
│       └── serialize.hpp          # Recommended entry point
│
└── tests/
    ├── test_format_abstraction.cpp   # Foundation layer tests
    ├── test_asyncle_format.cpp       # Integration layer tests
    └── test_format_isolation.cpp     # Zero-coupling verification ⭐
```

---

## Design Principles

### 1. Zero Coupling

asyncle is completely isolated from implementation libraries (simdjson/Glaze):

```bash
# Verify zero coupling
grep -r "simdjson" include/asyncle/format/  # No matches
grep -r "glaze" include/asyncle/format/     # No matches
```

**Mechanisms**:
- **Type Alias** for JSON parsing (compile-time selection)
- **CPO + ADL** for serialization (ADL-based dispatch)

### 2. Zero Overhead

- Type alias: Compile-time selection, zero runtime cost
- CPO + ADL: Fully inlined, zero runtime cost
- No virtual functions, no type erasure

### 3. Layered Design

```
asyncle::format (Integration Layer - zero-coupled)
    ↓ uses abstract interfaces
format (Foundation Layer - thin wrappers)
    ↓ type alias / CPO
simdjson / Glaze (Implementation - optional)
```

### 4. Pluggable Implementation

Select implementations via CMake:
```bash
cmake -B build -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON
```

---

## Testing

### Build and Run Tests

```bash
# Build
ninja -C build test_format_abstraction test_asyncle_format test_format_isolation

# Run
./build/test_format_isolation      # Verify zero coupling
./build/test_format_abstraction    # Test custom implementations
./build/test_asyncle_format        # Integration tests
```

### Verify Zero Coupling

```bash
cd include/asyncle/format
grep -r "simdjson" .  # Should have no matches
grep -r "glaze" .     # Should have no matches
```

---

## FAQ

### Why not use simdjson/Glaze directly in asyncle?

**Answer**: For zero coupling and replaceability. Through Type Alias and CPO mechanisms, asyncle is completely unaware of specific implementations and they can be freely replaced.

### Does abstraction impact performance?

**Answer**: No. Type alias and CPO + ADL are zero-cost abstractions, fully inlined by the compiler.

### Can I use multiple JSON parsers simultaneously?

**Answer**: Yes in the foundation layer, but asyncle layer uses a single `format::json::parser` (type alias). For multiple parsers, use the foundation layer directly.

### How do I select which implementation to use?

**Answer**: Via CMake flags:
```bash
cmake -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON ..
```

### What happens if no implementation is enabled?

**Answer**: `format::json::parser` becomes a stub. Code compiles but returns errors at runtime.

---

## Contributing

### Adding a New JSON Parser

1. Create adapter in `include/format/json/` (e.g., `myparser.hpp`)
2. Implement `json_parser` concept
3. Add type alias in `include/format/json/parser.hpp`:
   ```cpp
   #ifdef FORMAT_HAS_MYPARSER
       using parser = myparser_document;
   #endif
   ```
4. No changes needed in asyncle code!

### Adding a New Serializer

1. Create adapter in `include/format/serialize/`
2. Implement `save_impl`/`load_impl` (found via ADL)
3. No changes needed in asyncle code!

---

## References

- [simdjson](https://github.com/simdjson/simdjson) - SIMD-accelerated JSON parser
- [Glaze](https://github.com/stephenberry/glaze) - Compile-time reflection serialization
- C++20/23 Customization Point Objects (CPO)
- Argument-Dependent Lookup (ADL)

---

**Last Updated**: 2025-10-08
