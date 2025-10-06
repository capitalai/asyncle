#ifndef ASYNCLE_HARDWARE_ARCH_DETECT_HPP
#define ASYNCLE_HARDWARE_ARCH_DETECT_HPP

// Compile-time CPU architecture detection
// This determines which architecture-specific header to include

// x86-64 / AMD64
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64) \
  || defined(_M_AMD64)
#define ASYNCLE_ARCH_X86_64 1
#define ASYNCLE_ARCH_NAME   "x86-64"

// x86 32-bit
#elif defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define ASYNCLE_ARCH_X86  1
#define ASYNCLE_ARCH_NAME "x86"

// ARM 64-bit (AArch64)
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define ASYNCLE_ARCH_AARCH64 1
#define ASYNCLE_ARCH_NAME    "aarch64"

// Apple Silicon detection (subset of AArch64)
#if defined(__APPLE__)
#define ASYNCLE_ARCH_APPLE_SILICON 1
#endif

// ARM 32-bit
#elif defined(__arm__) || defined(_M_ARM)
#define ASYNCLE_ARCH_ARM  1
#define ASYNCLE_ARCH_NAME "arm"

// RISC-V 64-bit
#elif defined(__riscv) && (__riscv_xlen == 64)
#define ASYNCLE_ARCH_RISCV64 1
#define ASYNCLE_ARCH_NAME    "riscv64"

// RISC-V 32-bit
#elif defined(__riscv) && (__riscv_xlen == 32)
#define ASYNCLE_ARCH_RISCV32 1
#define ASYNCLE_ARCH_NAME    "riscv32"

// PowerPC 64-bit
#elif defined(__powerpc64__) || defined(__ppc64__)
#define ASYNCLE_ARCH_PPC64 1
#define ASYNCLE_ARCH_NAME  "ppc64"

// Unknown architecture
#else
#define ASYNCLE_ARCH_UNKNOWN 1
#define ASYNCLE_ARCH_NAME    "unknown"
#warning "Unknown CPU architecture - using generic defaults"
#endif

// OS detection
#if defined(__linux__)
#define ASYNCLE_OS_LINUX 1
#define ASYNCLE_OS_NAME  "Linux"
#elif defined(_WIN32) || defined(_WIN64)
#define ASYNCLE_OS_WINDOWS 1
#define ASYNCLE_OS_NAME    "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
#define ASYNCLE_OS_MACOS 1
#define ASYNCLE_OS_NAME  "macOS"
#elif defined(__FreeBSD__)
#define ASYNCLE_OS_FREEBSD 1
#define ASYNCLE_OS_NAME    "FreeBSD"
#else
#define ASYNCLE_OS_UNKNOWN 1
#define ASYNCLE_OS_NAME    "Unknown"
#endif

#endif  // ASYNCLE_HARDWARE_ARCH_DETECT_HPP
