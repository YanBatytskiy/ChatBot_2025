file(GLOB_RECURSE shared_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(shared_lib ${shared_SRC})

target_include_directories(shared_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
