cmake_minimum_required(VERSION 3.16)

# Library for helper functions

# Collect all implementation sources under src/ (exclude main.cpp and demo_phase4.cpp)
file(GLOB_RECURSE MUDOP_IMPL_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
list(REMOVE_ITEM MUDOP_IMPL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/demo_phase4.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/demo_persistence_writer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/demo_persistence_reader.cpp
)

add_library(mudop_utils STATIC ${MUDOP_IMPL_SOURCES})
target_include_directories(mudop_utils
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_features(mudop_utils PUBLIC cxx_std_17)

# Main executable
add_executable(mudopdb main.cpp)
target_link_libraries(mudopdb PRIVATE mudop_utils)

# Phase 4 Demo executable
add_executable(demo_phase4 demo_phase4.cpp)
target_link_libraries(demo_phase4 PRIVATE mudop_utils)

# Persistence test executables
add_executable(persistence_writer demo_persistence_writer.cpp)
target_link_libraries(persistence_writer PRIVATE mudop_utils)

add_executable(persistence_reader demo_persistence_reader.cpp)
target_link_libraries(persistence_reader PRIVATE mudop_utils)
