cmake_minimum_required(VERSION 3.16)
project(Shark2_0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# ===== Сборка core в динамическую библиотеку =====
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS ${SRC_DIR}/core/*.cpp)
add_library(core SHARED ${CORE_SRC})
target_include_directories(core PUBLIC
  ${SRC_DIR}
  ${SRC_DIR}/core
)

# ===== Сборка client =====
file(GLOB_RECURSE CLIENT_SRC CONFIGURE_DEPENDS
  ${SRC_DIR}/client/*.cpp
  ${SRC_DIR}/dto/*.cpp
  ${SRC_DIR}/server/server_session.cpp
  ${SRC_DIR}/server/0_init_system.cpp
)
add_executable(client ${CLIENT_SRC})
target_include_directories(client PRIVATE
  ${SRC_DIR}
  ${SRC_DIR}/client
  ${SRC_DIR}/dto
  ${SRC_DIR}/core
)
target_link_libraries(client PRIVATE core)

# ===== Сборка server =====
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
target_link_libraries(server PRIVATE core)
