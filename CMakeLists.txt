# File: CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(FFmpegDirectXPlayer)

set(CMAKE_CXX_STANDARD 17)
include_directories("D:\\jigarpatel-cpp\\include")
link_directories("D:\\jigarpatel-cpp\\lib")

file(GLOB SOURCES "src/*.cpp")

add_executable(player WIN32  ${SOURCES})
target_link_libraries(player
    avformat
    avcodec
    avutil
    swscale
    d3d11
    dxgi
    d3dcompiler
    user32
    gdi32
)
