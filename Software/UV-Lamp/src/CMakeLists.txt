# Znajdź wszystkie pliki źródłowe w katalogu src
file(GLOB SOURCES
    "*.cpp"
    "*.hpp"
    "*.h"
)

# Utwórz bibliotekę z plików w katalogu src
add_library(uv_lib STATIC
    ${SOURCES}
)

# Linkuj bibliotekę z Pico SDK
target_link_libraries(uv_lib 
    pico_stdlib
    hardware_gpio
    hardware_timer
    hardware_clocks
    hardware_watchdog
    hardware_irq
)

# Dodaj katalog src jako katalog include dla biblioteki
target_include_directories(uv_lib PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)