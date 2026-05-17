find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)

add_executable(allblue MACOSX_BUNDLE
    src/main.cpp
    src/main_window.hpp
    src/main_window.cpp
    src/switch_controller.hpp
    src/switch_controller.cpp
    src/tray_icon.hpp
    src/tray_icon.cpp
)

target_link_libraries(allblue PRIVATE Qt6::Core Qt6::Widgets)
