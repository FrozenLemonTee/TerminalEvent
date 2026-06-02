cmake_minimum_required(VERSION 3.20)

project(TerminalEvent VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lte_core
    src/input_parser.cpp
    src/input_reader.cpp
    src/native_event.cpp
    src/terminal_session.cpp
)
set_target_properties(lte_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(lte_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(lte SHARED
    src/c_api.cpp
)

target_link_libraries(lte PRIVATE lte_core)
target_include_directories(lte PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(lte PRIVATE LTE_BUILD_DLL)

add_library(lte_static STATIC
    src/c_api.cpp
)

target_link_libraries(lte_static PRIVATE lte_core)
target_include_directories(lte_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(lte_static PROPERTIES OUTPUT_NAME lte)

enable_testing()

add_executable(parser_test tests/parser_test.cpp)
target_link_libraries(parser_test PRIVATE lte_core)
add_test(NAME parser_test COMMAND parser_test)

add_executable(c_api_smoke_test tests/c_api_smoke_test.cpp)
target_link_libraries(c_api_smoke_test PRIVATE lte_static)
add_test(NAME c_api_smoke_test COMMAND c_api_smoke_test)
