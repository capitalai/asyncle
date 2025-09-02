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

add_library(asyncle INTERFACE)
target_include_directories(asyncle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Add compile features to ensure proper C++23 support
target_compile_features(asyncle INTERFACE cxx_std_23)

# L0 I/O library - compiled implementation
add_library(asyncle_io STATIC src/io/l0_linux.cpp)
target_include_directories(asyncle_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(asyncle_io PUBLIC cxx_std_23)
target_link_libraries(asyncle_io PUBLIC asyncle)

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
  
  # L0 I/O interface test
  add_executable(test_l0_io tests/test_l0_io.cpp)
  target_link_libraries(test_l0_io PRIVATE asyncle_io)
  add_test(NAME asyncle.l0_io COMMAND test_l0_io)
endif()
