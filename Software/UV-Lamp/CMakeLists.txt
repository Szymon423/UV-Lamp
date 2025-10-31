# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(UV-Lamp C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ============================================================================
# INCLUDE DIRECTORIES
# ============================================================================
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(./examples)
include_directories(./lib/Config)
include_directories(./lib/Fonts)
include_directories(./lib/GUI)
include_directories(./lib/LCD)
include_directories(./lib/lvgl)
include_directories(./ui)

# ============================================================================
# KONFIGURACJA LVGL - MUSI BYĆ PRZED add_subdirectory(lvgl)!
# ============================================================================

# Wyłącz przykłady LVGL
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1)
set(LV_CONF_BUILD_DISABLE_DEMOS 1)

# Dodaj subdirectories (LVGL jako ostatnie, żeby miało dostęp do wszystkich include)
add_subdirectory(src)
add_subdirectory(./lib/Config)
add_subdirectory(./lib/LCD)
add_subdirectory(./lib/Fonts)
add_subdirectory(./lib/GUI)
add_subdirectory(./examples)

# LVGL - dodaj PRZED jego add_subdirectory
add_subdirectory(./lib/lvgl)

# Konfiguracja LVGL - zaraz po add_subdirectory
target_compile_definitions(lvgl PUBLIC 
    LV_CONF_INCLUDE_SIMPLE
)

target_include_directories(lvgl PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lvgl PUBLIC
    pico_stdlib
    hardware_spi
    hardware_flash
)

# UI na końcu (zależy od LVGL)
add_subdirectory(./ui)

# ============================================================================
# EXECUTABLE
# ============================================================================
add_executable(UV-Lamp main.c)

pico_set_program_name(UV-Lamp "UV-Lamp")
pico_set_program_version(UV-Lamp "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(UV-Lamp 0)
pico_enable_stdio_usb(UV-Lamp 0)

# Add the standard library to the build
target_link_libraries(UV-Lamp 
    pico_stdlib
    hardware_spi
    hardware_flash
    hardware_pwm
    Config 
    examples
    uv_lib
    LCD 
    GUI 
    Fonts 
    lvgl
    ui
)
    
pico_add_extra_outputs(UV-Lamp)
