add_library(coil_compiler STATIC
    ${CMAKE_CURRENT_LIST_DIR}/compiler/frontend/lexer/Lexer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/frontend/parser/Parser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/frontend/parser/ParsingPrecedence.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/ast/Node.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/ast/visitors/Visitor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/ast/visitors/PrintVisitor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/middle-end/semantic/ConversionVisitor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/middle-end/semantic/RegisterVisitor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/middle-end/semantic/TypeCheckingVisitor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/middle-end/ir/IRGenerator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/io/FileHandler.cpp
)

target_include_directories(coil_compiler PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(coil_compiler PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(coil_compiler PRIVATE $<$<CONFIG:Debug>:COIL_DEBUG_BUILD>)

add_library(coil_ir STATIC
    ${CMAKE_CURRENT_LIST_DIR}/compiler/middle-end/ir/TAC.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/middle-end/ir/BasicBlock.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/middle-end/ir/IRPrint.cpp
)

target_include_directories(coil_ir PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(coil_ir PRIVATE -Wall -Wextra -Wpedantic)

add_library(coil_backend_alloc STATIC
    ${CMAKE_CURRENT_LIST_DIR}/compiler/backend/alloc/Liveness.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/backend/alloc/InterferenceGraph.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compiler/backend/alloc/GraphColoring.cpp
)

target_include_directories(coil_backend_alloc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(coil_backend_alloc PRIVATE -Wall -Wextra -Wpedantic)

add_library(coil_backend_codegen STATIC
    ${CMAKE_CURRENT_LIST_DIR}/compiler/backend/codegen/CodeGen.cpp
)

target_include_directories(coil_backend_codegen PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(coil_backend_codegen PRIVATE -Wall -Wextra -Wpedantic)
