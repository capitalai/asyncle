#ifndef ASYNCLE_IO_MEMORY_MAPPING_HPP
#define ASYNCLE_IO_MEMORY_MAPPING_HPP

#include "../../platform/mmap.hpp"

namespace asyncle::io {

// Re-export the platform mmap interface in the asyncle namespace
// This provides a clean asyncle-specific interface while using the
// cross-platform mmap library as the implementation

using memory_region = platform::mmap::memory_region;
using memory_request = platform::mmap::memory_request;
using memory_error = platform::mmap::memory_error;
using memory_caps = platform::mmap::memory_caps;

// Re-export all enums
using platform::mmap::access_mode;
using platform::mmap::sharing_mode;
using platform::mmap::backing_type;
using platform::mmap::placement_strategy;
using platform::mmap::page_preference;
using platform::mmap::commit_strategy;
using platform::mmap::populate_strategy;
using platform::mmap::locking_strategy;
using platform::mmap::sync_semantics;
using platform::mmap::access_pattern;
using platform::mmap::error_domain;
using platform::mmap::error_code;

// Re-export all functions
using platform::mmap::map_memory;
using platform::mmap::sync_memory;
using platform::mmap::unmap_memory;
using platform::mmap::query_capabilities;
using platform::mmap::advise_memory;
using platform::mmap::lock_memory;
using platform::mmap::unlock_memory;
using platform::mmap::prefetch_memory;

} // namespace asyncle::io

#endif