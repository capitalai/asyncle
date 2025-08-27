cmake_minimum_required(VERSION 3.23)
project(asyncle VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ASYNCLE_BUILD_TESTS "Build Asyncle tests" ON)

add_library(asyncle INTERFACE)
target_include_directories(asyncle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(ASYNCLE_BUILD_TESTS)
  enable_testing()
  
  # Concept-specific tests
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
endif()
