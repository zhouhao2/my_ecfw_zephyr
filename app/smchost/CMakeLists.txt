# SPDX-License-Identifier: Apache-2.0

target_sources_ifdef(CONFIG_SMCHOST app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/poc_smchost.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/acpi_region.h
    ${CMAKE_CURRENT_LIST_DIR}/smchost.h
    ${CMAKE_CURRENT_LIST_DIR}/smc.h
    ${CMAKE_CURRENT_LIST_DIR}/sci.h
    ${CMAKE_CURRENT_LIST_DIR}/scicodes.h
    )
