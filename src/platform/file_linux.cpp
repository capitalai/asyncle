#include "../../include/platform/file.hpp"

#ifdef __linux__
#include "../../include/platform/file_linux.hpp"

namespace platform::file {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Linux implementation of the cross-platform file interface
// These functions delegate to the platform-specific implementations

expected<file_handle, file_error> open_file(const char* path, const file_request& request) noexcept {
    return linux_impl::open_file_impl(path, request);
}

expected<file_handle, file_error> create_temp(const char* dir_path, const file_request& request) noexcept {
    return linux_impl::create_temp_impl(dir_path, request);
}

void close_file(file_handle& handle) noexcept { linux_impl::close_file_impl(handle); }

expected<file_info, file_error> stat_file(const file_handle& handle) noexcept {
    return linux_impl::stat_file_impl(handle);
}

expected<file_info, file_error> stat_path(const char* path, bool follow_symlinks) noexcept {
    return linux_impl::stat_path_impl(path, follow_symlinks);
}

expected<uint64_t, file_error> get_file_size(const file_handle& handle) noexcept {
    return linux_impl::get_file_size_impl(handle);
}

expected<io_result, file_error> read_file(const file_handle& handle, const io_request& request) noexcept {
    return linux_impl::read_file_impl(handle, request);
}

expected<io_result, file_error> write_file(const file_handle& handle, const io_request& request) noexcept {
    return linux_impl::write_file_impl(handle, request);
}

expected<io_result, file_error>
  read_vectored(const file_handle& handle, const io_request* requests, size_t count) noexcept {
    return linux_impl::read_vectored_impl(handle, requests, count);
}

expected<io_result, file_error>
  write_vectored(const file_handle& handle, const io_request* requests, size_t count) noexcept {
    return linux_impl::write_vectored_impl(handle, requests, count);
}

expected<uint64_t, file_error> seek_file(const file_handle& handle, int64_t offset, seek_origin origin) noexcept {
    return linux_impl::seek_file_impl(handle, offset, origin);
}

expected<uint64_t, file_error> tell_file(const file_handle& handle) noexcept {
    return linux_impl::tell_file_impl(handle);
}

expected<void, file_error> sync_file(const file_handle& handle, sync_flags flags) noexcept {
    return linux_impl::sync_file_impl(handle, flags);
}

expected<void, file_error>
  sync_range(const file_handle& handle, uint64_t offset, uint64_t length, sync_flags flags) noexcept {
    return linux_impl::sync_range_impl(handle, offset, length, flags);
}

expected<void, file_error> truncate_file(const file_handle& handle, uint64_t size) noexcept {
    return linux_impl::truncate_file_impl(handle, size);
}

expected<void, file_error> allocate_file(const file_handle& handle, uint64_t offset, uint64_t length) noexcept {
    return linux_impl::allocate_file_impl(handle, offset, length);
}

expected<void, file_error> deallocate_file(const file_handle& handle, uint64_t offset, uint64_t length) noexcept {
    return linux_impl::deallocate_file_impl(handle, offset, length);
}

expected<void, file_error> lock_file(const file_handle& handle, const file_lock& lock) noexcept {
    return linux_impl::lock_file_impl(handle, lock);
}

expected<file_lock, file_error> test_lock(const file_handle& handle, const file_lock& lock) noexcept {
    return linux_impl::test_lock_impl(handle, lock);
}

expected<void, file_error>
  advise_file(const file_handle& handle, uint64_t offset, uint64_t length, file_advice advice) noexcept {
    return linux_impl::advise_file_impl(handle, offset, length, advice);
}

expected<size_t, file_error> splice_files(
  const file_handle& in,
  uint64_t*          in_offset,
  const file_handle& out,
  uint64_t*          out_offset,
  size_t             length,
  uint32_t           flags) noexcept {
    return linux_impl::splice_files_impl(in, in_offset, out, out_offset, length, flags);
}

expected<size_t, file_error>
  sendfile_op(const file_handle& out, const file_handle& in, uint64_t* offset, size_t count) noexcept {
    return linux_impl::sendfile_op_impl(out, in, offset, count);
}

file_caps query_file_caps() noexcept { return linux_impl::query_file_caps_impl(); }

}  // namespace platform::file

#else   // Non-Linux platforms - provide stubs

namespace platform::file {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Stub implementations for non-Linux platforms
expected<file_handle, file_error> open_file(const char* path, const file_request& request) noexcept {
    return expected<file_handle, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<file_handle, file_error> create_temp(const char* dir_path, const file_request& request) noexcept {
    return expected<file_handle, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

void close_file(file_handle& handle) noexcept { handle.fd = -1; }

expected<file_info, file_error> stat_file(const file_handle& handle) noexcept {
    return expected<file_info, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<file_info, file_error> stat_path(const char* path, bool follow_symlinks) noexcept {
    return expected<file_info, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<uint64_t, file_error> get_file_size(const file_handle& handle) noexcept {
    return expected<uint64_t, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<io_result, file_error> read_file(const file_handle& handle, const io_request& request) noexcept {
    return expected<io_result, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<io_result, file_error> write_file(const file_handle& handle, const io_request& request) noexcept {
    return expected<io_result, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<io_result, file_error>
  read_vectored(const file_handle& handle, const io_request* requests, size_t count) noexcept {
    return expected<io_result, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<io_result, file_error>
  write_vectored(const file_handle& handle, const io_request* requests, size_t count) noexcept {
    return expected<io_result, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<uint64_t, file_error> seek_file(const file_handle& handle, int64_t offset, seek_origin origin) noexcept {
    return expected<uint64_t, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<uint64_t, file_error> tell_file(const file_handle& handle) noexcept {
    return expected<uint64_t, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<void, file_error> sync_file(const file_handle& handle, sync_flags flags) noexcept {
    return expected<void, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<void, file_error>
  sync_range(const file_handle& handle, uint64_t offset, uint64_t length, sync_flags flags) noexcept {
    return expected<void, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<void, file_error> truncate_file(const file_handle& handle, uint64_t size) noexcept {
    return expected<void, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<void, file_error> allocate_file(const file_handle& handle, uint64_t offset, uint64_t length) noexcept {
    return expected<void, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<void, file_error> deallocate_file(const file_handle& handle, uint64_t offset, uint64_t length) noexcept {
    return expected<void, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<void, file_error> lock_file(const file_handle& handle, const file_lock& lock) noexcept {
    return expected<void, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<file_lock, file_error> test_lock(const file_handle& handle, const file_lock& lock) noexcept {
    return expected<file_lock, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<void, file_error>
  advise_file(const file_handle& handle, uint64_t offset, uint64_t length, file_advice advice) noexcept {
    return expected<void, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<size_t, file_error> splice_files(
  const file_handle& in,
  uint64_t*          in_offset,
  const file_handle& out,
  uint64_t*          out_offset,
  size_t             length,
  uint32_t           flags) noexcept {
    return expected<size_t, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

expected<size_t, file_error>
  sendfile_op(const file_handle& out, const file_handle& in, uint64_t* offset, size_t count) noexcept {
    return expected<size_t, file_error>(unexpect, file_error(error_domain::platform, error_code::not_supported));
}

file_caps query_file_caps() noexcept { return file_caps {}; }

}  // namespace platform::file

#endif  // __linux__

