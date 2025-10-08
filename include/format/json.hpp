#ifndef FORMAT_JSON_HPP
#define FORMAT_JSON_HPP

// format::json - JSON parsing foundation layer
//
// This is a convenience header that includes all format::json components.
//
// Design:
// - Foundation layer (low-level, sync, thin wrappers)
// - Type alias selects implementation at compile time
// - Zero coupling to specific libraries in asyncle layer
//
// Components:
// - format::json::parser        - Unified parser (type alias)
// - format::json::error         - Error types
// - format::json::parser_caps   - Parser capabilities
// - format::json::result<T>     - Result type
//
// Usage:
//   // Direct use of foundation layer
//   #include <format/json.hpp>
//
//   format::json::parser parser(json_string);
//   auto doc = parser.iterate();
//
//   // Or use asyncle layer (recommended)
//   #include <asyncle/format/json.hpp>
//
//   auto parser = asyncle::format::json::make_parser()
//       .source(json_string)
//       .make();

#include "json/concepts.hpp"  // json_parser concept
#include "json/parser.hpp"    // Unified parser (type alias)
#include "json/types.hpp"     // error, result, parser_caps

// Note: Implementation (simdjson.hpp) is included by json/parser.hpp
// based on feature flags.

namespace format::json {

// Re-export key types for convenience
using error_type = error;
using caps_type  = parser_caps;

// Unified parser type (implementation selected at compile time)
// - If FORMAT_HAS_SIMDJSON: parser = simdjson_document
// - Otherwise: parser = stub_parser (compile-time error helper)
//
// This is what asyncle::format::json uses internally.
//
// Note: For serialization, use format::serialize (which can use Glaze).

}  // namespace format::json

#endif  // FORMAT_JSON_HPP
