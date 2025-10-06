cmake_minimum_required(VERSION 3.16)
project(OpenMP_Mini1_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use system default compiler (Apple Clang on macOS)
# Removed preference for Homebrew LLVM clang++ due to ABI compatibility issues

# Make interface headers available
include_directories(${PROJECT_SOURCE_DIR}/interface)

# Collect shared sources into a library
set(CORE_SOURCES
  src/populationModel.cpp
  src/readcsv.cpp
  src/service.cpp
  src/populationModelColumn.cpp
  src/service_column.cpp
  src/utils.cpp
  src/benchmark_utils.cpp
  src/benchmark_runner.cpp
  src/fireRowModel.cpp
  src/fireColumnModel.cpp
  src/fire_service_row.cpp
  src/fire_service_column.cpp
)

add_library(openmp_core STATIC ${CORE_SOURCES})
target_compile_features(openmp_core PRIVATE cxx_std_17)
target_compile_options(openmp_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(openmp_core PRIVATE c++)

# Application
add_executable(${PROJECT_NAME}_app src/main.cpp)
target_compile_features(${PROJECT_NAME}_app PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME}_app PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME}_app PRIVATE openmp_core)

# Synthetic row-wise benchmark
add_executable(${PROJECT_NAME}_row_benchmark src/synthetic_row_benchmark.cpp)
target_compile_features(${PROJECT_NAME}_row_benchmark PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME}_row_benchmark PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME}_row_benchmark PRIVATE openmp_core)

# Basic unit tests
add_executable(${PROJECT_NAME}_tests tests/basic_tests.cpp)
target_compile_features(${PROJECT_NAME}_tests PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME}_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME}_tests PRIVATE openmp_core)

# Fire data model test
add_executable(${PROJECT_NAME}_fire_test src/fire_test.cpp)
target_compile_features(${PROJECT_NAME}_fire_test PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME}_fire_test PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME}_fire_test PRIVATE openmp_core)

# Fire column model test
add_executable(${PROJECT_NAME}_fire_column_test src/fire_column_test.cpp)
target_compile_features(${PROJECT_NAME}_fire_column_test PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME}_fire_column_test PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME}_fire_column_test PRIVATE openmp_core)

# Fire analytics benchmark
add_executable(${PROJECT_NAME}_fire_analytics src/fire_analytics_test.cpp)
target_compile_features(${PROJECT_NAME}_fire_analytics PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME}_fire_analytics PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME}_fire_analytics PRIVATE openmp_core)

# Require OpenMP (fail configuration if not available)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  message(STATUS "OpenMP found: enabling via FindOpenMP")
  target_link_libraries(openmp_core PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(openmp_core PRIVATE OPENMP_ENABLED=1)
else()
  # Try to locate libomp installed by Homebrew on macOS (common path)
  if(APPLE)
    set(HOMEBREW_LIBOMP_DIR /opt/homebrew/opt/libomp)
    find_path(HOMEBREW_LIBOMP_INCLUDE_DIR omp.h HINTS ${HOMEBREW_LIBOMP_DIR}/include)
    find_library(HOMEBREW_LIBOMP_LIB NAMES omp libomp HINTS ${HOMEBREW_LIBOMP_DIR}/lib)
    if(HOMEBREW_LIBOMP_INCLUDE_DIR AND HOMEBREW_LIBOMP_LIB)
      message(STATUS "OpenMP: using Homebrew libomp at ${HOMEBREW_LIBOMP_DIR}")
  target_include_directories(openmp_core PRIVATE ${HOMEBREW_LIBOMP_INCLUDE_DIR})
  # For AppleClang we need to pass -Xpreprocessor -fopenmp
  target_compile_options(openmp_core PRIVATE -Xpreprocessor -fopenmp)
  target_link_libraries(openmp_core PRIVATE ${HOMEBREW_LIBOMP_LIB})
  target_compile_definitions(openmp_core PRIVATE OPENMP_ENABLED=1)
    else()
      message(FATAL_ERROR "OpenMP not found. Install libomp (brew install libomp) or ensure your compiler supports OpenMP.")
    endif()
  else()
    message(FATAL_ERROR "OpenMP not found. Please install OpenMP for your platform.")
  endif()
endif()

message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
