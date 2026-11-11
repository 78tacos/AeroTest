# AeroTest — educational CAN bus simulator.
# Read this file with docs/cmake.md. The comments here are the short version.

cmake_minimum_required(VERSION 3.25)

project(AeroTest
  VERSION 0.1.0
  DESCRIPTION "Educational avionics bus simulator and test runner"
  LANGUAGES C CXX
)

# Default for every target we define. FetchContent dependencies keep their own.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# A hand-written `cmake -S . -B build` still emits compile_commands.json.
# Presets do not have to repeat this.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ninja is single-config: an empty build type means no debug symbols.
# -D and presets already fill the cache, so this only covers a bare configure.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug or Release" FORCE)
endif()

option(AEROTEST_BUILD_TESTS "Build GoogleTest targets" ON)
option(AEROTEST_WARNINGS "Turn on a useful warning set" ON)
option(AEROTEST_WERROR "Treat warnings as errors" OFF)
option(AEROTEST_SANITIZERS "AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Directory-wide so the fetched GoogleTest library is instrumented too.
# Putting the flags only on our targets would link unsanitized gtest into a
# sanitized binary and fail.
if(AEROTEST_SANITIZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "AEROTEST_SANITIZERS supports GCC and Clang only")
  endif()
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Semicolons inside the generator expression become separate compiler arguments.
function(aerotest_set_warnings target)
  if(NOT AEROTEST_WARNINGS)
    return()
  endif()
  target_compile_options(${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion>
  )
  if(AEROTEST_WERROR)
    set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
  endif()
endfunction()

# @ONLY so only @VAR@ is replaced. The version string then matches project().
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/aerotest/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/aerotest/version.hpp
  @ONLY
)

# This tree does not use C++20 modules. CMake 3.28 otherwise scans sources for
# them, and Clang's scan needs clang-scan-deps, which is not part of the CI
# install. yaml-cpp's own project() call turns the scan on for its target, so
# the property is forced off again after it is fetched.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# yaml-cpp is linked by the scenario parser. Public headers do not include it.
# Its tests, tools, contrib, and install rules stay off. docs/cmake.md walks
# each line. The parse function itself is still a stub.
include(FetchContent)
FetchContent_Declare(
  yaml-cpp
  URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.9.0.tar.gz
  URL_HASH SHA256=25cb043240f828a8c51beb830569634bc7ac603978e0f69d6b63558dadefd49a
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)
# yaml-cpp's project() can turn the scan back on. Set it off again before our
# targets are created, and pin the property on the target it already created.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set_target_properties(yaml-cpp PROPERTIES CXX_SCAN_FOR_MODULES OFF)
# yaml-cpp 0.9 headers trip Clang -Wsign-conversion. Marking the include
# system keeps that warning out of our build, including under AEROTEST_WERROR.
get_target_property(aerotest_yaml_includes yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(yaml-cpp PROPERTIES
  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${aerotest_yaml_includes}"
)

# Sources are listed on purpose. GLOB would miss a file you add until a
# reconfigure, and a new file is easy to forget.
add_library(aerotest
  src/status.cpp
  src/message.cpp
  src/loopback_bus.cpp
  src/socketcan_bus.cpp
  src/fault_injector.cpp
  src/test_runner.cpp
  src/scenario_parser.cpp
  src/air_data_subsystem.cpp
  src/engine_subsystem.cpp
  src/scenario_runner.cpp
  src/results_writer.cpp
)
target_link_libraries(aerotest PRIVATE yaml-cpp::yaml-cpp)
# The namespaced name is what tests link, same as an installed package would.
add_library(aerotest::aerotest ALIAS aerotest)

target_include_directories(aerotest PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)
target_compile_features(aerotest PUBLIC cxx_std_20)
aerotest_set_warnings(aerotest)

add_executable(aerotest_cli apps/aerotest_cli/main.cpp)
set_target_properties(aerotest_cli PROPERTIES OUTPUT_NAME aerotest)
target_link_libraries(aerotest_cli PRIVATE aerotest::aerotest)
aerotest_set_warnings(aerotest_cli)

if(AEROTEST_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
