cmake_minimum_required(VERSION 3.20)
project(AtlusSaveConverter VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
)
FetchContent_MakeAvailable(imgui)

# ImGui Sources
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_compile_definitions(imgui_lib PUBLIC UNICODE _UNICODE)

add_library(miniz_lib STATIC src/core/miniz.c src/core/miniz.h)
target_include_directories(miniz_lib PUBLIC src/core)


# Application Sources
set(APP_SOURCES
    src/main.cpp
    src/core/P3RCrypt.hpp
    src/core/SMT5VCrypt.hpp
    src/core/P3PCrypt.hpp
    src/core/SH2Crypt.hpp
    src/core/P4GCrypt.hpp
    src/core/P5RCrypt.hpp
    src/core/P5SCrypt.hpp
    src/core/P5TCrypt.hpp
    src/core/miniz.h
    src/core/miniz.c
    src/core/SteamDetector.hpp
    src/models/GameProfile.hpp
    src/ui/Theme.hpp
    src/ui/Theme.cpp
    src/ui/FileDialog.hpp
    src/ui/FileDialog.cpp
    src/ui/MainWindow.hpp
    src/ui/MainWindow.cpp
)

add_executable(AtlusSaveConverter WIN32 ${APP_SOURCES})

target_include_directories(AtlusSaveConverter PRIVATE
    src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_definitions(AtlusSaveConverter PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(AtlusSaveConverter PRIVATE
    imgui_lib
    miniz_lib
    d3d11
    d3dcompiler
    dxgi
    shell32
    ole32
    comctl32
    shlwapi
    bcrypt
)

# MSVC specific flags
if(MSVC)
    target_compile_options(AtlusSaveConverter PRIVATE /W4 /permissive- /utf-8)
endif()

# Automated unit tests
add_executable(test_p3rcrypt tests/test_p3rcrypt.cpp src/core/P3RCrypt.hpp)
target_include_directories(test_p3rcrypt PRIVATE src)
if(MSVC)
    target_compile_options(test_p3rcrypt PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_smt5vcrypt tests/test_smt5vcrypt.cpp src/core/SMT5VCrypt.hpp)
target_include_directories(test_smt5vcrypt PRIVATE src)
target_link_libraries(test_smt5vcrypt PRIVATE bcrypt)
if(MSVC)
    target_compile_options(test_smt5vcrypt PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_p3pcrypt tests/test_p3pcrypt.cpp src/core/P3PCrypt.hpp)
target_include_directories(test_p3pcrypt PRIVATE src)
target_link_libraries(test_p3pcrypt PRIVATE bcrypt)
if(MSVC)
    target_compile_options(test_p3pcrypt PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_sh2crypt tests/test_sh2crypt.cpp src/core/SH2Crypt.hpp)
target_include_directories(test_sh2crypt PRIVATE src)
if(MSVC)
    target_compile_options(test_sh2crypt PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_p4gcrypt tests/test_p4gcrypt.cpp src/core/P4GCrypt.hpp)
target_include_directories(test_p4gcrypt PRIVATE src)
target_link_libraries(test_p4gcrypt PRIVATE bcrypt)
if(MSVC)
    target_compile_options(test_p4gcrypt PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_steamdetector tests/test_steamdetector.cpp src/core/SteamDetector.hpp)
target_include_directories(test_steamdetector PRIVATE src)
if(MSVC)
    target_compile_options(test_steamdetector PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(sync_p4g_to_steam tests/sync_p4g_to_steam.cpp src/core/P4GCrypt.hpp)
target_include_directories(sync_p4g_to_steam PRIVATE src)
target_link_libraries(sync_p4g_to_steam PRIVATE bcrypt)
if(MSVC)
    target_compile_options(sync_p4g_to_steam PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_p5rcrypt tests/test_p5rcrypt.cpp src/core/P5RCrypt.hpp)
target_include_directories(test_p5rcrypt PRIVATE src)
target_link_libraries(test_p5rcrypt PRIVATE bcrypt miniz_lib)
if(MSVC)
    target_compile_options(test_p5rcrypt PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_p5scrypt tests/test_p5scrypt.cpp src/core/P5SCrypt.hpp)
target_include_directories(test_p5scrypt PRIVATE src)
if(MSVC)
    target_compile_options(test_p5scrypt PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(test_p5tcrypt tests/test_p5tcrypt.cpp src/core/P5TCrypt.hpp)
target_include_directories(test_p5tcrypt PRIVATE src)
target_link_libraries(test_p5tcrypt PRIVATE miniz_lib)
if(MSVC)
    target_compile_options(test_p5tcrypt PRIVATE /W4 /permissive- /utf-8)
endif()



