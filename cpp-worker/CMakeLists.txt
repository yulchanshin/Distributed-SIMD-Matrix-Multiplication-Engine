#cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake_minimum_required(VERSION 3.14)

project(MatrixMultiplicationOptimization VERSION 1.0 LANGUAGES CXX)

# --- C++ standard settings (apply globally to every target below) ---
set(CMAKE_CXX_STANDARD 20)          # Compile against C++20.
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Hard-fail if the compiler can't do C++20
set(CMAKE_CXX_EXTENSIONS OFF)       # Use -std=c++20, not -std=gnu++20
                                    # (portable, no GNU-only extensions).

# Default to a Release build. Our optimization flags below only fire under the
# Release config; without this, a plain `cmake -B build` on a single-config
# generator (Make/Ninja) leaves CMAKE_BUILD_TYPE empty -> no -O3 -> benchmark
# numbers measured against unoptimized code. Skip for multi-config generators
# (Visual Studio/Xcode), which pick the config at build time instead.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Optimize for the build host's exact CPU (enables AVX/AVX-512 etc. for matmul).
# ON by default since this is a benchmarking project; turn OFF (-march not added)
# if the binary must run on a different machine than the one that built it,
# otherwise it may SIGILL on unsupported instructions.
option(MATMUL_NATIVE_ARCH "Optimize for the build host CPU (-march=native)" ON)

include(FetchContent)

# Turn OFF Google Benchmark's own test suite BEFORE we build it, so it doesn't
# also pull in GoogleTest and compile benchmark's internal tests we don't need.
# CACHE + FORCE is how you override an option that a subproject defines.
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

# Describe WHERE to get the dependency and WHICH version. Pinning a tag
# (not a branch) keeps the build reproducible across machines/time.
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.5
)

# Actually clone + build Google Benchmark as part of our build. This creates
# the target we link against below. Lives in build/_deps/ — nothing is
# installed system-wide.
FetchContent_MakeAvailable(benchmark)

# Define our benchmark program. Named matrix_benchmark 
add_executable(matrix_benchmark benchmarks/benchmark_main.cpp)

# Link our executable against Google Benchmark. The :: form is the modern
# "imported target" — it carries the library's include paths + flags for us.
target_link_libraries(matrix_benchmark PRIVATE benchmark::benchmark)

# Compiler Flags. -O3 only in Release; -march=native additionally gated on the
# MATMUL_NATIVE_ARCH option so it stays a deliberate, portable-by-default choice.
target_compile_options(matrix_benchmark PRIVATE
    -Wall
    $<$<CONFIG:Release>:-O3>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${MATMUL_NATIVE_ARCH}>>:-march=native>)
