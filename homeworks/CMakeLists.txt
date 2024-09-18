cmake_minimum_required(VERSION 3.25)

project(Performanc-aware-programming C CXX)


add_executable(
    # executable name
    x86_simulator
    
    # source files compield into the executbale
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)