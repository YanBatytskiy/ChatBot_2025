cmake_minimum_required(VERSION 3.16)
project(Shark3_0_Postgres_Client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
add_compile_options($<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-g>)

find_package(Threads REQUIRED)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SRC_DIR ${PROJECT_ROOT}/src)

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
)

file(GLOB_RECURSE CLIENT_SRC CONFIGURE_DEPENDS ${SRC_DIR}/client/*.cpp)
add_executable(client ${CLIENT_SRC})
target_include_directories(client PRIVATE
  ${SRC_DIR}
  ${SRC_DIR}/client
  ${SRC_DIR}/dto
  ${SRC_DIR}/core
  ${NLOHMANN_JSON_INCLUDE_DIR}
)
target_link_libraries(client PRIVATE core_local dto_local sqlite3 Threads::Threads)
