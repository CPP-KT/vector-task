cmake_minimum_required(VERSION 3.24...3.31)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ConfigureCompiler)
include(CompilerWarnings)

project(vector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

ct_configure_compiler()
include(Dependencies)

# Setup a 'solution' target
file(GLOB SOLUTION_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(LENGTH SOLUTION_SOURCES SOLUTION_SOURCES_LENGTH)
if(SOLUTION_SOURCES_LENGTH EQUAL 0)
  add_library(solution INTERFACE)
  target_include_directories(solution INTERFACE src)
else()
  add_library(solution ${SOLUTION_SOURCES})
  target_include_directories(solution PUBLIC src)
  set_target_properties(solution PROPERTIES LINKER_LANGUAGE CXX)
  ct_set_compiler_warnings(solution)
endif()

# Setup a 'tests' target
file(GLOB TESTS_SOURCES CONFIGURE_DEPENDS test/*.cpp)
add_executable(tests ${TESTS_SOURCES})
target_include_directories(tests PRIVATE test)
ct_set_compiler_warnings(tests)

# Link tests with solution
target_link_libraries(tests PRIVATE solution)

# Link tests with dependencies
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
