SET(SOURCES 
    components/ui_comp_image1.c
    components/ui_comp.c
    screens/ui_ScreenPowerSetting.c
    screens/ui_ScreenTimerSetting.c
    screens/ui_ScreenMain.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
)

add_library(ui ${SOURCES})

target_link_libraries(ui PUBLIC lvgl)

target_include_directories(ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/screens
    ${CMAKE_CURRENT_SOURCE_DIR}/components
)
