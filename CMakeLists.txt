cmake_minimum_required(VERSION 3.20)

project(sse3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    execute_process(COMMAND "xcrun --show-sdk-path" OUTPUT_VARIABLE MACOSX_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

set(BUILD_SHARED_LIBS OFF)

find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

if (APPLE)
    set(FREETYPE_LIBRARY_PATH "/Users/vyacheslav/.brew/opt/freetype/lib/libfreetype.dylib")
    set(GLFW_LIBRARY_PATH     "/Users/vyacheslav/.brew/opt/glfw/lib/libglfw.dylib")
    set(ASSIMP_LIBRARY_PATH   "/Users/vyacheslav/.brew/opt/assimp/lib/libassimp.dylib")
    set(LIBPNG_LIBRARY_PATH   "/Users/vyacheslav/.brew/opt/libpng/lib/libpng.dylib")
endif()

add_library(glad src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(sse3 main.cpp stb_image.cpp)

if (APPLE)
    set_target_properties(sse3 PROPERTIES MACOSX_BUNDLE true)
    set_target_properties(sse3 PROPERTIES BUILD_RPATH "@executable_path/../Frameworks" INSTALL_RPATH "@executable_path/../Frameworks")

    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/build/sse3.app/Contents/Frameworks)

    add_custom_command(TARGET sse3 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/resources" "sse3.app/Contents/Resources/resources")
    add_custom_command(TARGET sse3 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/shaders"   "sse3.app/Contents/Resources/shaders")

    execute_process(COMMAND realpath ${FREETYPE_LIBRARY_PATH} OUTPUT_VARIABLE FREETYPE_ORIG_LIB_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND realpath ${GLFW_LIBRARY_PATH}     OUTPUT_VARIABLE GLFW_ORIG_LIB_PATH     OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND realpath ${ASSIMP_LIBRARY_PATH}   OUTPUT_VARIABLE ASSIMP_ORIG_LIB_PATH   OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND realpath ${LIBPNG_LIBRARY_PATH}   OUTPUT_VARIABLE LIBPNG_ORIG_LIB_PATH   OUTPUT_STRIP_TRAILING_WHITESPACE)

    file(COPY ${FREETYPE_ORIG_LIB_PATH} DESTINATION ${CMAKE_SOURCE_DIR}/build/sse3.app/Contents/Frameworks)
    file(COPY ${GLFW_ORIG_LIB_PATH}     DESTINATION ${CMAKE_SOURCE_DIR}/build/sse3.app/Contents/Frameworks)
    file(COPY ${ASSIMP_ORIG_LIB_PATH}   DESTINATION ${CMAKE_SOURCE_DIR}/build/sse3.app/Contents/Frameworks)
    file(COPY ${LIBPNG_ORIG_LIB_PATH}   DESTINATION ${CMAKE_SOURCE_DIR}/build/sse3.app/Contents/Frameworks)

    get_filename_component(FREETYPE_LIBRARY_FILENAME "${FREETYPE_ORIG_LIB_PATH}" NAME)
    get_filename_component(GLFW_LIBRARY_FILENAME     "${GLFW_ORIG_LIB_PATH}"     NAME)
    get_filename_component(ASSIMP_LIBRARY_FILENAME   "${ASSIMP_ORIG_LIB_PATH}"   NAME)
    get_filename_component(LIBPNG_LIBRARY_FILENAME   "${LIBPNG_ORIG_LIB_PATH}"   NAME)

    add_custom_command(TARGET sse3 POST_BUILD COMMAND install_name_tool -change "/Users/vyacheslav/.brew/opt/glfw/lib/libglfw.3.dylib"         "@rpath/${GLFW_LIBRARY_FILENAME}"     "sse3.app/Contents/MacOS/sse3")
    add_custom_command(TARGET sse3 POST_BUILD COMMAND install_name_tool -change "/Users/vyacheslav/.brew/opt/freetype/lib/libfreetype.6.dylib" "@rpath/${FREETYPE_LIBRARY_FILENAME}" "sse3.app/Contents/MacOS/sse3")
    add_custom_command(TARGET sse3 POST_BUILD COMMAND install_name_tool -change "/Users/vyacheslav/.brew/opt/assimp/lib/libassimp.6.dylib"     "@rpath/${ASSIMP_LIBRARY_FILENAME}"   "sse3.app/Contents/MacOS/sse3")
    add_custom_command(TARGET sse3 POST_BUILD COMMAND install_name_tool -change "/Users/vyacheslav/.brew/opt/libpng/lib/libpng16.16.dylib"     "@rpath/${LIBPNG_LIBRARY_FILENAME}"   "sse3.app/Contents/Frameworks/${FREETYPE_LIBRARY_FILENAME}")

    add_custom_command(TARGET sse3 POST_BUILD COMMAND codesign --force --deep --sign - "${CMAKE_SOURCE_DIR}/build/sse3.app")
endif()

target_include_directories(sse3 PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(sse3 PRIVATE glad OpenGL::GL glfw Freetype::Freetype assimp::assimp)

#target_compile_options(sse3 PRIVATE -fsanitize=address -g -O0)
#target_link_options(sse3 PRIVATE -fsanitize=address)
