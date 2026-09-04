# SPDX-License-Identifier: Apache-2.0

target_sources_ifdef(CONFIG_PWRMGMT app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/poc_pwrseq.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/pwrplane.h
    )
