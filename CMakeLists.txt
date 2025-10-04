cmake_minimum_required(VERSION 3.16)
project(OpenMP_Mini1_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer clang++ if available but allow overriding via CLI
if(NOT CMAKE_CXX_COMPILER)
  find_program(CLANGPP clang++)
  if(CLANGPP)
    set(CMAKE_CXX_COMPILER ${CLANGPP} CACHE STRING "C++ compiler" FORCE)
  endif()
endif()

# Make interface headers available
include_directories(${PROJECT_SOURCE_DIR}/interface)

# Collect sources
set(SOURCES
  src/main.cpp
  src/my_interface.cpp
)

add_executable(${PROJECT_NAME}_app ${SOURCES})

target_compile_features(${PROJECT_NAME}_app PRIVATE cxx_std_17)

target_compile_options(${PROJECT_NAME}_app PRIVATE -Wall -Wextra -Wpedantic)

message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
