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


target_include_directories(matrix_benchmark PRIVATE include)
# Link our executable against Google Benchmark. The :: form is the modern
# "imported target" — it carries the library's include paths + flags for us.
target_link_libraries(matrix_benchmark PRIVATE benchmark::benchmark)

# Compiler Flags. -O3 only in Release; -march=native additionally gated on the
# MATMUL_NATIVE_ARCH option so it stays a deliberate, portable-by-default choice.
target_compile_options(matrix_benchmark PRIVATE
    -Wall
    $<$<CONFIG:Release>:-O3>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${MATMUL_NATIVE_ARCH}>>:-march=native>)

# --- Correctness tests (TSK-102 acceptance criterion #1) ---
# A separate, dependency-free executable that asserts the math is correct.
# No Google Benchmark link needed; it only consumes the header-only Matrix.
enable_testing()
add_executable(matrix_tests tests/matrix_tests.cpp)
target_include_directories(matrix_tests PRIVATE include)
target_compile_options(matrix_tests PRIVATE -Wall -Wextra)

# Register with CTest so `ctest` (or `cmake --build build --target test`) runs it.
add_test(NAME matrix_tests COMMAND matrix_tests)

# --- TSK-202: C++ gRPC server ---
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Where protoc drops its output. Inside the build tree (an artifact, not source)
# so it stays gitignored and regenerable — never mixed into include/ or src/.
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})  # protoc won't create its own out dir

# The .proto lives at the repo root, one level up from cpp-worker/.
get_filename_component(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../proto" ABSOLUTE)
set(PROTO_FILE "${PROTO_DIR}/matrix.proto")

# The files protoc produces, named after the .proto's basename.
set(GENERATED_SRCS
    "${GENERATED_DIR}/matrix.pb.cc"
    "${GENERATED_DIR}/matrix.grpc.pb.cc")
set(GENERATED_HDRS
    "${GENERATED_DIR}/matrix.pb.h"
    "${GENERATED_DIR}/matrix.grpc.pb.h")

# Run protoc at build time. This is exactly the invocation from generate.sh,
# but CMake owns it now. DEPENDS on the .proto means it re-runs only when the
# schema changes; listing these files as OUTPUT lets CMake wire the dependency
# so they're generated before matrix_server compiles.
add_custom_command(
    OUTPUT ${GENERATED_SRCS} ${GENERATED_HDRS}
    COMMAND protobuf::protoc
    ARGS --proto_path="${PROTO_DIR}"
         --cpp_out="${GENERATED_DIR}"
         --grpc_out="${GENERATED_DIR}"
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
    COMMENT "Generating C++ protobuf + gRPC sources from matrix.proto"
    VERBATIM)

# The server: our entrypoint + the generated message/service code. Listing the
# generated .cc files here is what pulls the add_custom_command into the build.
add_executable(matrix_server src/main.cpp ${GENERATED_SRCS})

# include/ for our Matrix header; GENERATED_DIR so #include "matrix.pb.h" resolves.
target_include_directories(matrix_server PRIVATE include ${GENERATED_DIR})

# grpc++ transitively brings in the rest of gRPC + Abseil; libprotobuf is named
# explicitly because main.cpp uses protobuf message types directly.
target_link_libraries(matrix_server PRIVATE gRPC::grpc++ protobuf::libprotobuf)

# Match the benchmark target's optimization flags (this is a perf project).
# No -Wextra: the generated protobuf code is noisy and we don't own it.
target_compile_options(matrix_server PRIVATE
    -Wall
    $<$<CONFIG:Release>:-O3>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${MATMUL_NATIVE_ARCH}>>:-march=native>)
