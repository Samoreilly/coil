cmake_minimum_required(VERSION 4.3)
project(coil_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

find_package(fmt REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/../compiler_sources.cmake)

add_executable(coil ${CMAKE_CURRENT_LIST_DIR}/../main.cpp)
target_link_libraries(coil PRIVATE fmt::fmt coil_compiler coil_ir coil_backend_codegen)

add_executable(test_runner ${CMAKE_CURRENT_LIST_DIR}/test_runner.cpp)

add_executable(tac_tests ${CMAKE_CURRENT_LIST_DIR}/ir/tac_tests.cpp)
add_executable(ir_generator_smoke ${CMAKE_CURRENT_LIST_DIR}/ir/ir_generator_smoke.cpp)
add_executable(ir_printer_tests ${CMAKE_CURRENT_LIST_DIR}/ir/ir_printer_tests.cpp)
add_executable(ir_cli_smoke ${CMAKE_CURRENT_LIST_DIR}/ir/ir_cli_smoke.cpp)
add_executable(alloc_tests ${CMAKE_CURRENT_LIST_DIR}/backend/alloc_tests.cpp)
add_executable(codegen_tests ${CMAKE_CURRENT_LIST_DIR}/backend/codegen_tests.cpp)
add_executable(asm_cli_smoke ${CMAKE_CURRENT_LIST_DIR}/backend/asm_cli_smoke.cpp)

target_include_directories(test_runner PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(test_runner PRIVATE fmt::fmt coil_compiler)
target_include_directories(tac_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(tac_tests PRIVATE fmt::fmt coil_compiler coil_ir)
target_include_directories(ir_generator_smoke PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ir_generator_smoke PRIVATE fmt::fmt coil_compiler coil_ir)
target_include_directories(ir_printer_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ir_printer_tests PRIVATE fmt::fmt coil_compiler coil_ir)
target_include_directories(ir_cli_smoke PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ir_cli_smoke PRIVATE fmt::fmt coil_compiler coil_ir)
target_include_directories(alloc_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(alloc_tests PRIVATE fmt::fmt coil_backend_alloc coil_ir)
target_include_directories(codegen_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(codegen_tests PRIVATE fmt::fmt coil_backend_codegen coil_ir)
target_include_directories(asm_cli_smoke PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(asm_cli_smoke PRIVATE fmt::fmt coil_backend_codegen coil_ir)
