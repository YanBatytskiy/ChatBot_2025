cmake_minimum_required(VERSION 3.16)
project(Shark2_0_Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
add_compile_options($<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-g>)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SRC_DIR ${PROJECT_ROOT}/src)

file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS ${SRC_DIR}/core/*.cpp)
add_library(core_local STATIC ${CORE_SRC})
target_include_directories(core_local PUBLIC
  ${SRC_DIR}
  ${SRC_DIR}/core
)

file(GLOB_RECURSE SERVER_SRC CONFIGURE_DEPENDS
  ${SRC_DIR}/server/*.cpp
  ${SRC_DIR}/dto/*.cpp
)
add_executable(server ${SERVER_SRC})
target_include_directories(server PRIVATE
  ${SRC_DIR}
  ${SRC_DIR}/server
  ${SRC_DIR}/dto
  ${SRC_DIR}/core
)
target_link_libraries(server PRIVATE core_local)
