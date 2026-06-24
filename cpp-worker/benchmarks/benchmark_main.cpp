#include <benchmark/benchmark.h>

static void BM_returnOne(benchmark::State& state) {
    for (const auto& _ : state) {
        int x = 1;
    }
}

BENCHMARK(BM_returnOne);

BENCHMARK_MAIN();
