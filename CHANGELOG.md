# Changelog

All notable changes to the asyncle project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added - 2025-10-08

#### Format Layer Zero-Coupling Design

**Major architectural improvement**: Complete isolation of asyncle from JSON/serialization implementations.

**New Files**:
- `docs/adr/001-format-layer-zero-coupling.md` - Architecture Decision Record
- `docs/README.md` - Documentation index and quick start guide
- `docs/format_zero_coupling.md` - Zero-coupling design details
- `docs/format_architecture.md` - Overall architecture
- `docs/format_customization.md` - Customization guide
- `include/format/json/parser.hpp` - Type alias for implementation selection
- `include/asyncle/format/json.hpp` - Zero-coupled JSON operations (redesigned)
- `include/asyncle/format/serialize.hpp` - Zero-coupled serialization (redesigned)
- `tests/test_format_isolation.cpp` - Verification test

**Design Principles**:
1. **Zero Coupling**: asyncle never references simdjson/Glaze
   - No `#ifdef FORMAT_HAS_SIMDJSON` in asyncle code
   - No simdjson/Glaze type references
   - Verified by grep: zero matches

2. **Zero Overhead**:
   - Type alias: compile-time selection
   - CPO + ADL: fully inlined
   - No virtual functions, no type erasure

3. **Pluggable**:
   - Implementation selected at compile time via CMake flags
   - Users can provide custom implementations
   - asyncle code unchanged when switching implementations

**Mechanisms**:
- **Type Alias** (JSON): `format::json::parser` = selected implementation
- **CPO + ADL** (Serialization): `format::serialize::save/load` via ADL lookup

**Verification**:
```bash
./tests/test_format_isolation
# ✓ asyncle::format::json has zero coupling to simdjson
# ✓ asyncle::format::serialize has zero coupling to Glaze
```

### Changed - 2025-10-08

**File Reorganization**:
- Replaced `asyncle/format/json.hpp` (old, coupled version)
- Replaced `asyncle/format/serialize.hpp` (old, coupled version)
- Removed `asyncle/format/json_v2.hpp` (experimental)
- Updated `tests/test_format_isolation.cpp` to use new headers

**Documentation Updates**:
- Added superseded warnings to:
  - `FORMAT_LIBRARY.md` → superseded by ADR-001
  - `FORMAT_JSON_EXAMPLES.md` → superseded by ADR-001
  - `FUTURE_ASYNCLE_JSON.md` → superseded by format_architecture.md
  - `JSON_PARSER_ANALYSIS.md` → superseded by ADR-001

### Migration Guide

**For Users**:

Old code (coupled):
```cpp
#include <asyncle/format/json.hpp>

// Had to know about simdjson_tag
auto parser = asyncle::format::json::parser<simdjson_tag>()
    .source(json)
    .make();
```

New code (zero-coupled):
```cpp
#include <asyncle/format/json.hpp>

// No knowledge of implementation
auto parser = asyncle::format::json::make_parser()
    .source(json)
    .make();
```

**For Developers**:

No changes needed! The new design is a drop-in replacement with better abstraction.

Implementation selection remains the same:
```bash
cmake -DFORMAT_ENABLE_SIMDJSON=ON ..
cmake -DFORMAT_ENABLE_GLAZE=ON ..
```

### Architecture Summary

```
Before (Coupled):
  asyncle::format::json
    ├─ #ifdef FORMAT_HAS_SIMDJSON  ❌
    ├─ simdjson_tag                 ❌
    └─ parser<simdjson_tag>()       ❌

After (Zero-Coupled):
  asyncle::format::json
    ├─ uses format::json::parser    ✓ (type alias)
    ├─ make_parser()                ✓ (no template)
    └─ zero simdjson references     ✓
```

### Testing

All tests pass:
- ✅ `test_format_abstraction` - Concepts and custom implementations
- ✅ `test_format_json` - Concrete implementations (simdjson/Glaze)
- ✅ `test_asyncle_format` - Integration tests
- ✅ `test_format_isolation` - Zero-coupling verification

---

## [0.0.1] - 2024-XX-XX

Initial development version.

### Added
- Core asyncle operation concepts
- Platform abstraction layer (file, mmap, process)
- Hardware abstraction layer (memory, CPU detection)
- I/O operations
- Format library foundation (JSON, serialization)

---

[Unreleased]: https://github.com/yourorg/asyncle/compare/v0.0.1...HEAD
[0.0.1]: https://github.com/yourorg/asyncle/releases/tag/v0.0.1
