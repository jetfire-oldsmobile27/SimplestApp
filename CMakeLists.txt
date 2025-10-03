cmake_minimum_required(VERSION 3.16)

project(SimplestApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Core Multimedia)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_INCLUDE_DIR ${CONAN_BOOST_ROOT}/include)
set(Boost_LIBRARY_DIR ${CONAN_BOOST_ROOT}/lib)
find_package(Boost COMPONENTS date_time)
if(Boost_FOUND)
    message(STATUS "=====Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "=====Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "=====Boost_VERSION: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

find_library(_CAMERA2NDK_LIB camera2ndk)
    find_library(_MEDIANDK_LIB mediandk)
    find_library(_ANDROID_LIB android)

    if(NOT _CAMERA2NDK_LIB)
        set(_CAMERA2NDK_LIB "-lcamera2ndk")
    endif()
    if(NOT _MEDIANDK_LIB)
        set(_MEDIANDK_LIB "-lmediandk")
    endif()
    if(NOT _ANDROID_LIB)
        set(_ANDROID_LIB "-landroid")
    endif()

find_package(OpenCV REQUIRED)


qt_add_executable(appSimplestApp
    main.cpp
    CameraController.h
    CameraController.cpp
)

qt_add_qml_module(appSimplestApp
    URI SimplestApp
    VERSION 1.0
    QML_FILES
        Main.qml
)

qt_add_android_permission(appSimplestApp
        NAME android.permission.CAMERA
    )

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appSimplestApp PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appSimplestApp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
target_link_libraries(appSimplestApp
    PRIVATE Qt6::Quick 
    Qt6::CorePrivate 
    Qt6::Multimedia
    ${Boost_LIBRARIES} 
    ${OpenCV_LIBS}
    $<$<BOOL:${ANDROID}>:${_CAMERA2NDK_LIB}>
    $<$<BOOL:${ANDROID}>:${_MEDIANDK_LIB}>
    $<$<BOOL:${ANDROID}>:${_ANDROID_LIB}>
)

include(GNUInstallDirs)
install(TARGETS appSimplestApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
