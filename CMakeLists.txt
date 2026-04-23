cmake_minimum_required(VERSION 3.20)

project(sse3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(APPLE)
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOSX_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(CMAKE_OSX_SYSROOT "${MACOSX_SDK_PATH}")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

message(STATUS "Freetype include dirs: ${FREETYPE_INCLUDE_DIRS}")

execute_process(COMMAND cmd /c dir "$ENV{GITHUB_WORKSPACE}\\vcpkg\\installed\\x64-windows\\include\\freetype" /s /b OUTPUT_VARIABLE dir_output)
message(STATUS "include dir: ${dir_output}")

add_library(glad src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(sse3 main.cpp stb_image.cpp)

target_include_directories(sse3 PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(sse3 PRIVATE glad OpenGL::GL glfw Freetype::Freetype assimp::assimp)

if(WIN32)
    set_target_properties(sse3 PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
