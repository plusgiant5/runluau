cmake_minimum_required(VERSION 3.10)
project(runluau LANGUAGES CXX C)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNIX")
elseif(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWINDOWS")
endif()

include_directories(shared)
# TODO: not working lol
add_library(Luau.Compiler STATIC IMPORTED)
add_library(Luau STATIC IMPORTED)

# Add your source files
add_executable(runluau main.cpp execute.cpp plugins.cpp)
