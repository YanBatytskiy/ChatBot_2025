cmake_minimum_required(VERSION 3.16)
project(Shark3_0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

find_package(PostgreSQL REQUIRED)
find_package(Threads REQUIRED)

if(APPLE)
  set(NLOHMANN_JSON_INCLUDE_DIR "/opt/homebrew/include")
else()
  set(NLOHMANN_JSON_INCLUDE_DIR "/usr/include")
endif() 

# ===== dto =====
file(GLOB_RECURSE DTO_SRC CONFIGURE_DEPENDS ${SRC_DIR}/dto/*.cpp)
if(DTO_SRC)
  add_library(dto STATIC ${DTO_SRC})
  target_include_directories(dto PUBLIC ${SRC_DIR} ${SRC_DIR}/dto ${NLOHMANN_JSON_INCLUDE_DIR})
else()
  add_library(dto INTERFACE)
  target_include_directories(dto INTERFACE ${SRC_DIR} ${SRC_DIR}/dto ${NLOHMANN_JSON_INCLUDE_DIR})
endif()

# ===== core (с PostgreSQL) =====
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS ${SRC_DIR}/core/*.cpp)
add_library(core SHARED ${CORE_SRC})
target_include_directories(core
  PUBLIC
    ${SRC_DIR} ${SRC_DIR}/core
    ${NLOHMANN_JSON_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
)
if(TARGET PostgreSQL::PostgreSQL)
  target_link_libraries(core PUBLIC dto PostgreSQL::PostgreSQL)
else()
  target_link_libraries(core PUBLIC dto ${PostgreSQL_LIBRARIES})
endif()

# ===== client =====
file(GLOB_RECURSE CLIENT_SRC CONFIGURE_DEPENDS ${SRC_DIR}/client/*.cpp)
add_executable(client ${CLIENT_SRC})
target_include_directories(client PRIVATE ${SRC_DIR} ${SRC_DIR}/client ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(client PRIVATE core dto)
target_link_libraries(client PRIVATE sqlite3 Threads::Threads)

# ===== server =====
file(GLOB_RECURSE SERVER_SRC CONFIGURE_DEPENDS ${SRC_DIR}/server/*.cpp)
add_executable(server ${SERVER_SRC})
target_include_directories(server PRIVATE
  ${SRC_DIR} ${SRC_DIR}/server
  ${NLOHMANN_JSON_INCLUDE_DIR} ${PostgreSQL_INCLUDE_DIRS}
)
if(TARGET PostgreSQL::PostgreSQL)
  target_link_libraries(server PRIVATE core dto PostgreSQL::PostgreSQL)
else()
  target_link_libraries(server PRIVATE core dto ${PostgreSQL_LIBRARIES})
endif()

set(CONFIG_DIR ".")
target_compile_definitions(server PRIVATE CONFIG_DIR="${CONFIG_DIR}")
