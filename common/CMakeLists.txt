add_library(opencraft_common STATIC
    nbt.c
    anvil.c
    chunk_codec.c
    level_data.c
)

target_include_directories(opencraft_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(opencraft_common PUBLIC ZLIB::ZLIB)
