file(GLOB_RECURSE DTO_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(dto_lib INTERFACE)
target_include_directories(dto_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
