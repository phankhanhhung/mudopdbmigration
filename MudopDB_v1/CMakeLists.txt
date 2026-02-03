cmake_minimum_required(VERSION 3.16)
project(MudopDB_v1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" OFF)

add_subdirectory(src)

if(BUILD_TESTS)
  # Enable CTest at the root so ctest discovers tests from subdirectories
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
