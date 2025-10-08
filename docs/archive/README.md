# Archived Documentation

This directory contains historical documentation that has been superseded by newer designs.

**These documents are kept for historical reference only.**

---

## Archived Documents

### Format Layer (Superseded 2025-10-08)

**Reason**: Design evolved to zero-coupling architecture.

| Document | Superseded By | Date |
|----------|---------------|------|
| [FORMAT_LIBRARY.md](FORMAT_LIBRARY.md) | [ADR-001](../adr/001-format-layer-zero-coupling.md) | 2025-10-08 |
| [FORMAT_JSON_EXAMPLES.md](FORMAT_JSON_EXAMPLES.md) | [docs/README.md](../README.md) | 2025-10-08 |
| [FUTURE_ASYNCLE_JSON.md](FUTURE_ASYNCLE_JSON.md) | [format_architecture.md](../format_architecture.md) | 2025-10-08 |
| [JSON_PARSER_ANALYSIS.md](JSON_PARSER_ANALYSIS.md) | [format_zero_coupling.md](../format_zero_coupling.md) | 2025-10-08 |

---

## Why Archived?

The format layer underwent a major redesign to achieve **zero coupling** between asyncle and external libraries (simdjson, Glaze).

**Key Changes**:
- asyncle no longer references simdjson/Glaze directly
- Type Alias + CPO mechanisms for abstraction
- Implementation selection at compile time

**Current Documentation**:
- [ADR-001: Format Layer Zero-Coupling Design](../adr/001-format-layer-zero-coupling.md) ⭐
- [format_zero_coupling.md](../format_zero_coupling.md)
- [format_architecture.md](../format_architecture.md)
- [format_customization.md](../format_customization.md)

---

## For Historical Context

If you need to understand the evolution of the design:

1. **Initial Analysis**: [JSON_PARSER_ANALYSIS.md](JSON_PARSER_ANALYSIS.md)
   - Why simdjson/Glaze were chosen
   - Performance considerations

2. **First Implementation**: [FORMAT_LIBRARY.md](FORMAT_LIBRARY.md)
   - Original foundation layer design
   - Had some coupling with asyncle

3. **Usage Examples**: [FORMAT_JSON_EXAMPLES.md](FORMAT_JSON_EXAMPLES.md)
   - How the old API worked

4. **Future Vision**: [FUTURE_ASYNCLE_JSON.md](FUTURE_ASYNCLE_JSON.md)
   - Planned async pipeline design
   - Builder pattern ideas

5. **Final Design**: [ADR-001](../adr/001-format-layer-zero-coupling.md) ⭐
   - Zero-coupling architecture
   - Type Alias + CPO mechanisms
   - Current implementation

---

**Note**: These documents may reference APIs and designs that no longer exist. Always refer to current documentation in the parent directory.
