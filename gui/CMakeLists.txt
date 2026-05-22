find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

include(FetchContent)
FetchContent_Declare(Qlementine GIT_REPOSITORY "https://github.com/oclero/qlementine.git" GIT_TAG master)
FetchContent_MakeAvailable(Qlementine)

set(CMAKE_AUTOMOC ON)

set(ALLBLUE_SOURCES
    src/main.cpp
    src/main_window.hpp
    src/main_window.cpp
    src/switch_controller.hpp
    src/switch_controller.cpp
    src/tray_icon.hpp
    src/tray_icon.cpp
)

if(APPLE)
    set_source_files_properties(resources/allblue.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    list(APPEND ALLBLUE_SOURCES resources/allblue.icns)
elseif(WIN32)
    list(APPEND ALLBLUE_SOURCES resources/allblue.rc)
endif()

add_executable(allblue MACOSX_BUNDLE ${ALLBLUE_SOURCES})

if(APPLE)
    set_target_properties(allblue PROPERTIES
        MACOSX_BUNDLE_ICON_FILE allblue.icns
        MACOSX_BUNDLE_BUNDLE_NAME "AllBlue"
    )
elseif(WIN32)
    set_target_properties(allblue PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_link_libraries(allblue PRIVATE Qt6::Core Qt6::Widgets qlementine)

qt_add_resources(allblue "resources"
    PREFIX "/"
    FILES
        resources/theme.json
        resources/allbluelogo.png
)
