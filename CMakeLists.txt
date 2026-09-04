# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Out-of-tree boards live under this repo (boards/linkedsemi/...).
# Must be set before find_package(Zephyr) so boards.cmake can find them.
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Must register BEFORE find_package(Zephyr): that is when post-build rules are
# collected. (add_custom_command on zephyr_final fails — different CMake dir.)
set(_LS_ECFW_FW_DESC_GEN "${CMAKE_CURRENT_LIST_DIR}/../ls_sdk/tools/leo/fw_desc_gen.py")
if(EXISTS "${_LS_ECFW_FW_DESC_GEN}")
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${Python3_EXECUTABLE} ${_LS_ECFW_FW_DESC_GEN}
      ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin
  )
  # Do not list zephyr.hex (Zephyr already emits it → ninja duplicate output).
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
    ${CMAKE_BINARY_DIR}/zephyr/zephyr_ec_into.bios
  )
else()
  message(WARNING "fw_desc_gen.py not found at ${_LS_ECFW_FW_DESC_GEN}")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ls_ecfw)

include(${CMAKE_CURRENT_LIST_DIR}/app/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/boards/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/drivers/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/misc/CMakeLists.txt)

include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_compile_options(-Wno-address-of-packed-member)
zephyr_linker_sources(SECTIONS linker/custom-sections.ld)
