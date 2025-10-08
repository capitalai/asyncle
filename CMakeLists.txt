cmake_minimum_required(VERSION 3.23)
project(asyncle VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce strict compiler version requirements for C++23
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0")
        message(FATAL_ERROR "GCC version must be at least 12.0 for C++23 support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "17.0")
        message(FATAL_ERROR "Clang version must be at least 17.0 for C++23 support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.35")
        message(FATAL_ERROR "MSVC version must be at least 19.35 (Visual Studio 2022 17.5) for C++23 support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
else()
    message(WARNING "Unknown compiler ${CMAKE_CXX_COMPILER_ID}. C++23 support may be incomplete.")
endif()

option(ASYNCLE_BUILD_TESTS "Build Asyncle tests" ON)
option(FORMAT_ENABLE_SIMDJSON "Enable simdjson JSON parser in format library" OFF)
option(FORMAT_ENABLE_GLAZE "Enable Glaze JSON parser in format library" OFF)
option(FORMAT_USE_FETCHCONTENT "Auto-fetch missing dependencies with FetchContent" OFF)

add_library(asyncle INTERFACE)
target_include_directories(asyncle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Add compile features to ensure proper C++23 support
target_compile_features(asyncle INTERFACE cxx_std_23)

# Format library - thin wrappers for data format parsers (JSON, etc.)
# Similar to platform library, this is a foundation layer
add_library(format INTERFACE)
target_include_directories(format INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(format INTERFACE cxx_std_23)

# Optional JSON parser support in format library
if(FORMAT_ENABLE_SIMDJSON)
    find_package(simdjson QUIET)
    if(simdjson_FOUND)
        message(STATUS "simdjson found (version ${simdjson_VERSION})")
        target_compile_definitions(format INTERFACE FORMAT_HAS_SIMDJSON)
        target_link_libraries(format INTERFACE simdjson::simdjson)
    elseif(FORMAT_USE_FETCHCONTENT)
        message(STATUS "simdjson not found, fetching via FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            simdjson
            GIT_REPOSITORY https://github.com/simdjson/simdjson.git
            GIT_TAG v3.10.1
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(simdjson)
        target_compile_definitions(format INTERFACE FORMAT_HAS_SIMDJSON)
        target_link_libraries(format INTERFACE simdjson::simdjson)
        message(STATUS "simdjson fetched successfully")
    else()
        message(WARNING "simdjson not found")
        message(WARNING "  System install: sudo apt install libsimdjson-dev")
        message(WARNING "  Or use FetchContent: cmake -DFORMAT_USE_FETCHCONTENT=ON")
        message(WARNING "  See: docs/EXTERNAL_DEPENDENCIES.md")
    endif()
endif()

if(FORMAT_ENABLE_GLAZE)
    find_package(glaze QUIET)
    if(glaze_FOUND)
        message(STATUS "Glaze found (version ${glaze_VERSION})")
        target_compile_definitions(format INTERFACE FORMAT_HAS_GLAZE)
        target_link_libraries(format INTERFACE glaze::glaze)
    elseif(FORMAT_USE_FETCHCONTENT)
        message(STATUS "Glaze not found, fetching via FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            glaze
            GIT_REPOSITORY https://github.com/stephenberry/glaze.git
            GIT_TAG v5.7.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(glaze)
        target_compile_definitions(format INTERFACE FORMAT_HAS_GLAZE)
        target_link_libraries(format INTERFACE glaze::glaze)
        message(STATUS "Glaze fetched successfully")
    else()
        message(WARNING "Glaze not found")
        message(WARNING "  Recommended: Use FetchContent: cmake -DFORMAT_USE_FETCHCONTENT=ON")
        message(WARNING "  Manual install: See docs/EXTERNAL_DEPENDENCIES.md")
    endif()
endif()

# Cross-platform mmap library - compiled implementation
add_library(platform_mmap STATIC src/platform/mmap_linux.cpp)
target_include_directories(platform_mmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(platform_mmap PUBLIC cxx_std_23)

# Cross-platform file library - compiled implementation
add_library(platform_file STATIC src/platform/file_linux.cpp)
target_include_directories(platform_file PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(platform_file PUBLIC cxx_std_23)

# Cross-platform process library - compiled implementation (Linux only for now)
if(UNIX AND NOT APPLE)
  add_library(platform_process STATIC src/platform/process_linux.cpp)
  target_include_directories(platform_process PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(platform_process PUBLIC cxx_std_23)
endif()

# Cross-platform hardware utilities library - compiled implementation
add_library(platform_hardware STATIC
    src/platform/hardware_linux.cpp
    src/platform/hardware_windows.cpp
    src/platform/hardware_macos.cpp
)
target_include_directories(platform_hardware PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(platform_hardware PUBLIC cxx_std_23)

if(ASYNCLE_BUILD_TESTS)
  enable_testing()
  
  # Core concept tests
  add_executable(test_basic_concepts tests/test_basic_concepts.cpp)
  target_link_libraries(test_basic_concepts PRIVATE asyncle)
  add_test(NAME asyncle.basic_concepts COMMAND test_basic_concepts)
  
  add_executable(test_value_concepts tests/test_value_concepts.cpp)
  target_link_libraries(test_value_concepts PRIVATE asyncle)
  add_test(NAME asyncle.value_concepts COMMAND test_value_concepts)
  
  add_executable(test_error_concepts tests/test_error_concepts.cpp)
  target_link_libraries(test_error_concepts PRIVATE asyncle)
  add_test(NAME asyncle.error_concepts COMMAND test_error_concepts)
  
  add_executable(test_operation_concepts tests/test_operation_concepts.cpp)
  target_link_libraries(test_operation_concepts PRIVATE asyncle)
  add_test(NAME asyncle.operation_concepts COMMAND test_operation_concepts)
  
  add_executable(test_utility_concepts tests/test_utility_concepts.cpp)
  target_link_libraries(test_utility_concepts PRIVATE asyncle)
  add_test(NAME asyncle.utility_concepts COMMAND test_utility_concepts)
  
  # New comprehensive tests for enhanced features
  add_executable(test_entries tests/test_entries.cpp)
  target_link_libraries(test_entries PRIVATE asyncle)
  add_test(NAME asyncle.entries COMMAND test_entries)
  
  add_executable(test_predicates tests/test_predicates.cpp)
  target_link_libraries(test_predicates PRIVATE asyncle)
  add_test(NAME asyncle.predicates COMMAND test_predicates)
  
  add_executable(test_command tests/test_command.cpp)
  target_link_libraries(test_command PRIVATE asyncle)
  add_test(NAME asyncle.command COMMAND test_command)
  
  add_executable(test_cpo tests/test_cpo.cpp)
  target_link_libraries(test_cpo PRIVATE asyncle)
  add_test(NAME asyncle.cpo COMMAND test_cpo)
  
  # Cross-platform mmap library test
  add_executable(test_platform_mmap tests/test_platform_mmap.cpp)
  target_link_libraries(test_platform_mmap PRIVATE platform_mmap)
  add_test(NAME platform.mmap COMMAND test_platform_mmap)
  
  # Cross-platform file library test
  add_executable(test_platform_file tests/test_platform_file.cpp)
  target_link_libraries(test_platform_file PRIVATE platform_file)
  add_test(NAME platform.file COMMAND test_platform_file)

  # Cross-platform process library test (Linux only for now)
  if(UNIX AND NOT APPLE)
    add_executable(test_platform_process tests/test_platform_process.cpp)
    target_link_libraries(test_platform_process PRIVATE platform_process)
    add_test(NAME platform.process COMMAND test_platform_process)
  endif()

  # Asyncle I/O modules test (Linux only - file/mmap implementations needed for other platforms)
  if(UNIX AND NOT APPLE)
    add_executable(test_asyncle_io tests/test_asyncle_io.cpp)
    target_link_libraries(test_asyncle_io PRIVATE asyncle platform_file platform_mmap)
    add_test(NAME asyncle.io COMMAND test_asyncle_io)
  endif()

  # Hardware memory utilities test
  add_executable(test_hardware_memory tests/test_hardware_memory.cpp)
  target_link_libraries(test_hardware_memory PRIVATE asyncle platform_hardware)
  add_test(NAME hardware.memory COMMAND test_hardware_memory)

  # Format library JSON parser tests (optional - requires external libraries)
  if(FORMAT_ENABLE_SIMDJSON OR FORMAT_ENABLE_GLAZE)
    add_executable(test_format_json tests/test_format_json.cpp)
    target_link_libraries(test_format_json PRIVATE format)
    add_test(NAME format.json COMMAND test_format_json)

    if(FORMAT_ENABLE_SIMDJSON AND FORMAT_ENABLE_GLAZE)
      message(STATUS "format::json tests enabled with both simdjson and Glaze")
    elseif(FORMAT_ENABLE_SIMDJSON)
      message(STATUS "format::json tests enabled with simdjson only")
    elseif(FORMAT_ENABLE_GLAZE)
      message(STATUS "format::json tests enabled with Glaze only")
    endif()
  else()
    message(STATUS "format::json tests SKIPPED (no parsers enabled)")
    message(STATUS "  Enable with: -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON")
  endif()

  # Format abstraction layer test (always available - tests concepts)
  add_executable(test_format_abstraction tests/test_format_abstraction.cpp)
  target_link_libraries(test_format_abstraction PRIVATE format)
  add_test(NAME format.abstraction COMMAND test_format_abstraction)
  message(STATUS "format abstraction tests enabled (tests concepts and custom implementations)")

  # Asyncle format integration test (tests asyncle <-> format integration)
  add_executable(test_asyncle_format tests/test_asyncle_format.cpp)
  target_link_libraries(test_asyncle_format PRIVATE asyncle format)
  add_test(NAME asyncle.format COMMAND test_asyncle_format)
  message(STATUS "asyncle::format integration tests enabled")

  # Format isolation test (verifies asyncle has zero coupling to simdjson/Glaze)
  add_executable(test_format_isolation tests/test_format_isolation.cpp)
  target_link_libraries(test_format_isolation PRIVATE asyncle format)
  add_test(NAME format.isolation COMMAND test_format_isolation)
  message(STATUS "format isolation tests enabled (verifies zero coupling)")
endif()
