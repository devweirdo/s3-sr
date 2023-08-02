# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(s3-sr)

idf_build_get_property(build_dir BUILD_DIR)
add_custom_target(sr_cmds_gen ALL COMMAND ${python} ${CMAKE_CURRENT_LIST_DIR}/sr_cmds_gen.py -d1 ${PROJECT_DIR} -d2 ${build_dir})
