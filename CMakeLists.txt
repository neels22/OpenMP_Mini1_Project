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
)

add_library(openmp_core STATIC ${CORE_SOURCES})
target_compile_features(openmp_core PRIVATE cxx_std_17)
target_compile_options(openmp_core PRIVATE -Wall -Wextra -Wpedantic)

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

# ═══════════════════════════════════════════════════════════════
# Air Quality Project - Parallel File Loading
# ═══════════════════════════════════════════════════════════════

# Air quality core sources
set(AIRQUALITY_SOURCES
  src/airquality_types.cpp
  src/parallel_csv_loader.cpp
)

# Parallel loading test executable
add_executable(AirQuality_ParallelTest
  src/test_parallel_loading.cpp
  ${AIRQUALITY_SOURCES}
)
target_compile_features(AirQuality_ParallelTest PRIVATE cxx_std_17)
target_compile_options(AirQuality_ParallelTest PRIVATE -Wall -Wextra -Wpedantic)
# Note: Will link OpenMP below

# Full air quality benchmark (complete implementation)
add_executable(AirQuality_Benchmark
  src/main_airquality_full.cpp
  src/airquality_types.cpp
  src/parallel_csv_loader.cpp
  src/airquality_model_row.cpp
  src/airquality_model_column.cpp
  src/airquality_service_row.cpp
  src/airquality_service_column.cpp
)
target_compile_features(AirQuality_Benchmark PRIVATE cxx_std_17)
target_compile_options(AirQuality_Benchmark PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(AirQuality_Benchmark PRIVATE /opt/homebrew/Cellar/llvm/21.1.0/lib/c++/libc++.1.0.dylib)
# Note: Will link OpenMP below

# Require OpenMP (fail configuration if not available)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  message(STATUS "OpenMP found: enabling via FindOpenMP")
  target_link_libraries(openmp_core PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(openmp_core PRIVATE OPENMP_ENABLED=1)
  # Link OpenMP to air quality test
  target_link_libraries(AirQuality_ParallelTest PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(AirQuality_ParallelTest PRIVATE OPENMP_ENABLED=1)
  # Link OpenMP to full benchmark
  target_link_libraries(AirQuality_Benchmark PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(AirQuality_Benchmark PRIVATE OPENMP_ENABLED=1)
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
  # Link OpenMP to air quality test
  target_include_directories(AirQuality_ParallelTest PRIVATE ${HOMEBREW_LIBOMP_INCLUDE_DIR})
  target_compile_options(AirQuality_ParallelTest PRIVATE -Xpreprocessor -fopenmp)
  target_link_libraries(AirQuality_ParallelTest PRIVATE ${HOMEBREW_LIBOMP_LIB})
  target_compile_definitions(AirQuality_ParallelTest PRIVATE OPENMP_ENABLED=1)
  # Link OpenMP to full benchmark
  target_include_directories(AirQuality_Benchmark PRIVATE ${HOMEBREW_LIBOMP_INCLUDE_DIR})
  target_compile_options(AirQuality_Benchmark PRIVATE -Xpreprocessor -fopenmp)
  target_link_libraries(AirQuality_Benchmark PRIVATE ${HOMEBREW_LIBOMP_LIB})
  target_compile_definitions(AirQuality_Benchmark PRIVATE OPENMP_ENABLED=1)
    else()
      message(FATAL_ERROR "OpenMP not found. Install libomp (brew install libomp) or ensure your compiler supports OpenMP.")
    endif()
  else()
    message(FATAL_ERROR "OpenMP not found. Please install OpenMP for your platform.")
  endif()
endif()

message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
