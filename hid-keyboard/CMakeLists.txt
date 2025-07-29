cmake_minimum_required(VERSION 3.22.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXTRA_MCUX_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../c2usb)
include("${SdkRootDirPath}/cmake/extension/mcux.cmake")
project(
    hid-keyboard
    LANGUAGES ASM C CXX
)
include("${SdkRootDirPath}/CMakeLists.txt")

mcux_add_macro(
    CC "-DUSB_STACK_BM"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../common"
)
mcux_add_source(
    SOURCES
        main.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    c2usb-example-hid
)

mcux_convert_binary(
    BINARY "${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin"
)
