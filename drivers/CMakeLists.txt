# SPDX-License-Identifier: Apache-2.0

if (CONFIG_PS2_KEYBOARD OR CONFIG_PS2_MOUSE)
target_sources(app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ps2kbaux.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/ps2kbaux.h
    )
endif()

target_sources_ifdef(CONFIG_KSCAN_EC app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/kbs_matrix.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/kbs_matrix.h
    ${CMAKE_CURRENT_LIST_DIR}/kbs_keymap.h
    )

target_sources_ifdef(CONFIG_POSTCODE_MANAGEMENT app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/max6958.c
    ${CMAKE_CURRENT_LIST_DIR}/port80display.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/max6958.h
    ${CMAKE_CURRENT_LIST_DIR}/port80display.h
    )

target_sources_ifdef(CONFIG_THERMAL_MANAGEMENT app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/adc_sensors.c
    ${CMAKE_CURRENT_LIST_DIR}/peci_hub.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/adc_sensors.h
    ${CMAKE_CURRENT_LIST_DIR}/peci_hub.h
    )

target_sources(app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/eeprom.c
    ${CMAKE_CURRENT_LIST_DIR}/i2c_hub.c
    ${CMAKE_CURRENT_LIST_DIR}/gpio_ls.c
    ${CMAKE_CURRENT_LIST_DIR}/fan_ls.c
    ${CMAKE_CURRENT_LIST_DIR}/acpi_ls.c
    ${CMAKE_CURRENT_LIST_DIR}/vci_stub.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/eeprom.h
    ${CMAKE_CURRENT_LIST_DIR}/i2c_hub.h
    ${CMAKE_CURRENT_LIST_DIR}/espi_hub.h
    ${CMAKE_CURRENT_LIST_DIR}/gpio_ec.h
    ${CMAKE_CURRENT_LIST_DIR}/fan.h
    ${CMAKE_CURRENT_LIST_DIR}/led.h
    ${CMAKE_CURRENT_LIST_DIR}/vci.h
    ${CMAKE_CURRENT_LIST_DIR}/acpi.h
    )

if(CONFIG_ESPI)
  target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/espi_hub.c)
else()
  target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/espi_hub_stub.c)
endif()

target_sources_ifdef(CONFIG_OOBMNGR_SUPPORT app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/espioob_mngr.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/espioob_mngr.h
    )

target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )
