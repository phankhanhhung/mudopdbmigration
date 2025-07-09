cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE MUDOP_IMPL_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
list(REMOVE_ITEM MUDOP_IMPL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/demo_phase4.cpp
)

add_library(mudop_utils STATIC ${MUDOP_IMPL_SOURCES})
target_include_directories(mudop_utils
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_features(mudop_utils PUBLIC cxx_std_17)

add_executable(mudopdb main.cpp)
target_link_libraries(mudopdb PRIVATE mudop_utils)

add_executable(demo_phase4 demo_phase4.cpp)
target_link_libraries(demo_phase4 PRIVATE mudop_utils)
