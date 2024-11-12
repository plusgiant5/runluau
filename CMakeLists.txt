cmake_minimum_required(VERSION 3.10)
project(runluau)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNIX")
elseif(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWINDOWS")
endif()

# Add your source files
add_executable(runluau main.cpp execute.cpp plugins.cpp)
