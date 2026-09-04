# SPDX-License-Identifier: Apache-2.0

target_sources(app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/board.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/board.h
    )

target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/linkedsemi
    )

if(CONFIG_BOARD_LS101X_EHL_TCP_3L)
target_sources(app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/linkedsemi/ls101x_ehl_tcp_3l.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/linkedsemi/ls101x_ehl_tcp_3l.h
    )
elseif(CONFIG_BOARD_LS101X_EVB OR CONFIG_SOC_LS1010)
target_sources(app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/linkedsemi/ls101x_evb.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/linkedsemi/ls101x_evb.h
    )
endif()
