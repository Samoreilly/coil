
cmake_minimum_required(VERSION 4.3)
project(coil)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

find_package(fmt)

include(${CMAKE_CURRENT_SOURCE_DIR}/compiler_sources.cmake)

add_executable(coil main.cpp)

target_link_libraries(coil PRIVATE fmt::fmt coil_compiler coil_ir coil_backend_codegen)
