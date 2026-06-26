#include "matrix.hpp"
#include <benchmark/benchmark.h>
#include <cstddef>
#include <vector>

static void BM_Naive_Matrix_Multiplication(benchmark::State& state) {
    std::size_t N = static_cast<std::size_t>(state.range(0));
    std::vector<float> ones(N * N, 1.0f);
    std::vector<float> ones2(N * N, 1.0f);

    const Matrix A(N, N, std::move(ones));
    const Matrix B(N, N, std::move(ones2));

    for (const auto& _ : state) {
        Matrix C = Matrix::multiply_matrix_naive(A, B);
        benchmark::DoNotOptimize(C);
    }
}

static void
BM_Naive_Matrix_Multiplication_Accumulator(benchmark::State& state) {
    std::size_t N = static_cast<std::size_t>(state.range(0));
    std::vector<float> ones(N * N, 1.0f);
    std::vector<float> ones2(N * N, 1.0f);

    const Matrix A(N, N, std::move(ones));
    const Matrix B(N, N, std::move(ones2));

    for (const auto& _ : state) {
        Matrix C = Matrix::multiply_matrix_accumulator(A, B);
        benchmark::DoNotOptimize(C);
    }
}

BENCHMARK(BM_Naive_Matrix_Multiplication)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024);
BENCHMARK(BM_Naive_Matrix_Multiplication_Accumulator)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024);

BENCHMARK_MAIN();
