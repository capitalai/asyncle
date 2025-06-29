cmake_minimum_required(VERSION 3.23)
project(asyncle VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ASYNCLE_BUILD_TESTS "Build Asyncle tests" ON)

add_library(asyncle INTERFACE)
target_include_directories(asyncle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(ASYNCLE_BUILD_TESTS)
  enable_testing()
  add_executable(tests tests/basic.cpp)
  target_link_libraries(tests PRIVATE asyncle)
  add_test(NAME asyncle.tests COMMAND tests)
endif()
