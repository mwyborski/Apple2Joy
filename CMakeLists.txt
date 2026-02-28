cmake_minimum_required(VERSION 3.13)

# --- Firmware build option (requires Pico SDK) ---
option(APPLE2JOY_BUILD_FIRMWARE "Build the USB-to-Apple-IIe adapter firmware" ON)

if(APPLE2JOY_BUILD_FIRMWARE)
    include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

    project(apple2joy C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    pico_sdk_init()

    # TJUH dependency (expected as git submodule in lib/tjuh/)
    if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/lib/tjuh/CMakeLists.txt)
        message(STATUS "TJUH not found — attempting automatic submodule initialization...")
        find_package(Git QUIET)
        if(GIT_FOUND)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- lib/tjuh
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                RESULT_VARIABLE git_result
                OUTPUT_VARIABLE git_output
                ERROR_VARIABLE  git_error
            )
            if(NOT git_result EQUAL 0)
                # Submodule entry may not exist yet — try adding it
                execute_process(
                    COMMAND ${GIT_EXECUTABLE} submodule add https://github.com/mwyborski/tjuh.git lib/tjuh
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                    RESULT_VARIABLE git_result
                )
            endif()
        endif()
        if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/lib/tjuh/CMakeLists.txt)
            message(FATAL_ERROR
                "TJUH could not be fetched automatically. "
                "Run manually: git submodule add https://github.com/mwyborski/tjuh.git lib/tjuh")
        endif()
    endif()
    add_subdirectory(lib/tjuh)
else()
    # Library-only mode: no Pico SDK project wrapper needed.
    # The consuming project is responsible for pico_sdk_init().
    project(apple2joy C)
endif()

# --- Apple2Joy library ---
add_library(apple2joy_lib INTERFACE)

target_sources(apple2joy_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/apple2joy.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mcp4251.c
)

target_include_directories(apple2joy_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# --- Firmware executable ---
if(APPLE2JOY_BUILD_FIRMWARE)
    add_executable(apple2joy_firmware
        firmware/main.c
    )

    target_link_libraries(apple2joy_firmware
        pico_stdlib
        tinyusb_host
        tinyusb_board
        hardware_spi
        tjuh
        apple2joy_lib
    )

    pico_add_extra_outputs(apple2joy_firmware)
endif()
