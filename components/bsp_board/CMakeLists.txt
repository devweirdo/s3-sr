idf_component_register(
    SRCS "bsp_board.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp-sr
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format -Wno-incompatible-pointer-types)
