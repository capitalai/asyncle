#ifndef ASYNCLE_HARDWARE_ARCH_CURRENT_HPP
#define ASYNCLE_HARDWARE_ARCH_CURRENT_HPP

#include "detect.hpp"

// Include architecture-specific constants based on detected architecture
#if defined(ASYNCLE_ARCH_X86_64) || defined(ASYNCLE_ARCH_X86)
#include "x86_64.hpp"

namespace asyncle::hardware::arch {
using current_arch = x86_64_info;
}  // namespace asyncle::hardware::arch
#elif defined(ASYNCLE_ARCH_AARCH64) || defined(ASYNCLE_ARCH_ARM)
#include "aarch64.hpp"

namespace asyncle::hardware::arch {
using current_arch = aarch64_info;
}  // namespace asyncle::hardware::arch
#else
#include "generic.hpp"

namespace asyncle::hardware::arch {
using current_arch = generic_info;
}  // namespace asyncle::hardware::arch
#endif

namespace asyncle::hardware {

// Import architecture constants into hardware namespace for convenience
using arch::current_arch;

// Architecture-specific cache line size constants
inline constexpr size_t cache_line_size    = current_arch::cache_line_size;
inline constexpr size_t l1_cache_line_size = current_arch::l1_cache_line_size;
inline constexpr size_t l2_cache_line_size = current_arch::l2_cache_line_size;
inline constexpr size_t l3_cache_line_size = current_arch::l3_cache_line_size;

}  // namespace asyncle::hardware

#endif  // ASYNCLE_HARDWARE_ARCH_CURRENT_HPP
