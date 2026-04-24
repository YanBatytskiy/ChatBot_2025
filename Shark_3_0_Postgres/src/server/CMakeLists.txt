cmake_minimum_required(VERSION 3.16)
project(Shark3_0_Postgres_Server LANGUAGES CXX)

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

if(APPLE AND NOT PostgreSQL_ROOT AND NOT DEFINED ENV{PostgreSQL_ROOT})
  foreach(_pg_prefix /opt/homebrew/opt/libpq /usr/local/opt/libpq)
    if(EXISTS "${_pg_prefix}")
      set(PostgreSQL_ROOT "${_pg_prefix}" CACHE PATH "Hint for find_package(PostgreSQL)")
      break()
    endif()
  endforeach()
endif()

find_package(PostgreSQL REQUIRED)

if(APPLE)
  set(NLOHMANN_JSON_INCLUDE_DIR "/opt/homebrew/include")
else()
  set(NLOHMANN_JSON_INCLUDE_DIR "/usr/include")
endif()

file(GLOB_RECURSE DTO_SRC CONFIGURE_DEPENDS ${SRC_DIR}/dto/*.cpp)
if(DTO_SRC)
  add_library(dto_local STATIC ${DTO_SRC})
  target_include_directories(dto_local PUBLIC ${SRC_DIR} ${SRC_DIR}/dto ${NLOHMANN_JSON_INCLUDE_DIR})
else()
  add_library(dto_local INTERFACE)
  target_include_directories(dto_local INTERFACE ${SRC_DIR} ${SRC_DIR}/dto ${NLOHMANN_JSON_INCLUDE_DIR})
endif()

file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS ${SRC_DIR}/core/*.cpp)
add_library(core_local STATIC ${CORE_SRC})
target_include_directories(core_local PUBLIC
  ${SRC_DIR}
  ${SRC_DIR}/core
  ${NLOHMANN_JSON_INCLUDE_DIR}
  ${PostgreSQL_INCLUDE_DIRS}
)

file(GLOB_RECURSE SERVER_SRC CONFIGURE_DEPENDS ${SRC_DIR}/server/*.cpp)
add_executable(server ${SERVER_SRC})
target_include_directories(server PRIVATE
  ${SRC_DIR}
  ${SRC_DIR}/server
  ${SRC_DIR}/dto
  ${SRC_DIR}/core
  ${NLOHMANN_JSON_INCLUDE_DIR}
  ${PostgreSQL_INCLUDE_DIRS}
)
if(TARGET PostgreSQL::PostgreSQL)
  target_link_libraries(server PRIVATE core_local dto_local PostgreSQL::PostgreSQL)
else()
  target_link_libraries(server PRIVATE core_local dto_local ${PostgreSQL_LIBRARIES})
endif()

set(CONFIG_DIR "${PROJECT_ROOT}/config")
target_compile_definitions(server PRIVATE CONFIG_DIR="${CONFIG_DIR}")
