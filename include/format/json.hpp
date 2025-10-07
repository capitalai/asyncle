#ifndef FORMAT_JSON_HPP
#define FORMAT_JSON_HPP

// format::json - JSON parsing library layer
//
// This is a format layer (similar to platform layer) that provides thin wrappers
// around external JSON libraries. The design philosophy:
//
// 1. Minimal abstraction - preserve native library functionality
// 2. Unified interface - standardize common operations across libraries
// 3. Capability exposure - let users query parser characteristics
// 4. No premature integration - operation-based async design comes later
//
// Supported libraries:
// - simdjson: High-performance streaming parser with SIMD optimization
// - Glaze: Compile-time reflection-based parser with structured binding
//
// Usage:
//   #include <format/json.hpp>
//
//   // simdjson for streaming market data
//   format::json::simdjson_parser parser;
//   auto doc = parser.parse(json_string);
//   if (doc) {
//       double price = doc.value()["price"].get_double();
//   }
//
//   // Glaze for structured configuration
//   format::json::glaze_parser parser;
//   auto config = parser.parse<Config>(json_string);
//
// Design notes:
// - This is NOT the final asyncle::json module with builder/make/try_take
// - This is the foundation layer, like platform::file vs asyncle::io::file
// - Future asyncle::json will build on top of this format layer
// - Async design (builder pattern, query objects, etc.) deferred to asyncle layer

#include "json/types.hpp"

// Include implementations based on feature flags
#ifdef FORMAT_HAS_SIMDJSON
#include "json/simdjson.hpp"
#endif

#ifdef FORMAT_HAS_GLAZE
#include "json/glaze.hpp"
#endif

namespace format::json {

// Re-export key types for convenience
using error_type = error;
using caps_type  = parser_caps;

}  // namespace format::json

#endif  // FORMAT_JSON_HPP
