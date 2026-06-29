// Correctness tests for the Matrix class (TSK-102, acceptance criterion #1).
//
// These prove the math is *right* — separate from the benchmark, which only
// proves it's fast. Every case uses integer-valued data so the float results
// are exactly representable; that lets us compare with == (no epsilon needed).
//
// Each multiply case is run through BOTH multiply variants, so we also confirm
// they agree with each other.

#include "matrix.hpp"

#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <vector>

// --- tiny test harness ---------------------------------------------------
static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            ++g_failures;                                                      \
            std::printf("  FAIL [%s:%d]: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                      \
    } while (0)

// Pointer to either multiply variant, so every case tests both.
using MultiplyFn = Matrix (*)(const Matrix&, const Matrix&);

// --- multiply test cases (run against both variants) ---------------------

// All-ones N x N: every output cell must equal N (the shared inner dimension).
static void test_all_ones(MultiplyFn mul) {
    const std::size_t N = 4;
    Matrix A(N, N, std::vector<float>(N * N, 1.0f));
    Matrix B(N, N, std::vector<float>(N * N, 1.0f));

    Matrix C = mul(A, B);

    CHECK(C.rows() == N);
    CHECK(C.cols() == N);
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            CHECK(C(i, j) == static_cast<float>(N));
}

// A * I == A. Catches indexing/stride bugs — a wrong operator() would not
// reproduce A unchanged.
static void test_identity(MultiplyFn mul) {
    Matrix A(3, 3, {1, 2, 3, 4, 5, 6, 7, 8, 9});

    Matrix I(3, 3); // zero-filled
    for (std::size_t d = 0; d < 3; ++d)
        I(d, d) = 1.0f;

    Matrix C = mul(A, I);

    CHECK(C.rows() == 3);
    CHECK(C.cols() == 3);
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            CHECK(C(i, j) == A(i, j));
}

// Hand-computed 2x2:
//   [1 2] [5 6]   [19 22]
//   [3 4] [7 8] = [43 50]
static void test_known_2x2(MultiplyFn mul) {
    Matrix A(2, 2, {1, 2, 3, 4});
    Matrix B(2, 2, {5, 6, 7, 8});

    Matrix C = mul(A, B);

    CHECK(C.rows() == 2);
    CHECK(C.cols() == 2);
    CHECK(C(0, 0) == 19.0f);
    CHECK(C(0, 1) == 22.0f);
    CHECK(C(1, 0) == 43.0f);
    CHECK(C(1, 1) == 50.0f);
}

// Non-square (2x3) * (3x2) -> (2x2). This is the important one: every
// benchmark is square, so a rows/cols swap would pass everything else but
// fail here.
//   [1 2 3] [ 7  8]   [ 58  64]
//   [4 5 6] [ 9 10] = [139 154]
//           [11 12]
static void test_non_square(MultiplyFn mul) {
    Matrix A(2, 3, {1, 2, 3, 4, 5, 6});
    Matrix B(3, 2, {7, 8, 9, 10, 11, 12});

    Matrix C = mul(A, B);

    CHECK(C.rows() == 2);
    CHECK(C.cols() == 2);
    CHECK(C(0, 0) == 58.0f);
    CHECK(C(0, 1) == 64.0f);
    CHECK(C(1, 0) == 139.0f);
    CHECK(C(1, 1) == 154.0f);
}

// Inner dimension 7 = one 4-wide SIMD step + a 3-element scalar tail. This is
// the case that exercises BOTH the vectorized loop and the remainder loop in
// the SIMD variant; every square / multiple-of-4 size skips the tail entirely,
// which is exactly where bugs hide. Cross-check against the naive baseline
// rather than hand-computing a 5x6 result. Values are small ints (exactly
// representable, and the SIMD reorders the summation), so == stays safe.
static void test_inner_dim_seven(MultiplyFn mul) {
    const std::size_t M = 5, K = 7, N = 6;
    std::vector<float> a(M * K), b(K * N);
    for (std::size_t idx = 0; idx < a.size(); ++idx)
        a[idx] = static_cast<float>((idx % 7) + 1);
    for (std::size_t idx = 0; idx < b.size(); ++idx)
        b[idx] = static_cast<float>((idx % 5) + 1);
    Matrix A(M, K, a), B(K, N, b);

    Matrix expected = Matrix::multiply_matrix_naive(A, B);
    Matrix C = mul(A, B);

    CHECK(C.rows() == M);
    CHECK(C.cols() == N);
    for (std::size_t i = 0; i < M; ++i)
        for (std::size_t j = 0; j < N; ++j)
            CHECK(C(i, j) == expected(i, j));
}

// Incompatible dims (A.cols() != B.rows()) must throw.
static void test_dimension_mismatch_throws(MultiplyFn mul) {
    Matrix A(2, 3, std::vector<float>(6, 1.0f));
    Matrix B(2, 2, std::vector<float>(4, 1.0f)); // 3 != 2

    bool threw = false;
    try {
        Matrix C = mul(A, B);
        (void)C;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

// --- cases independent of the multiply variant ---------------------------

// Constructor rejects a data vector whose size != rows * cols.
static void test_constructor_validation() {
    bool threw = false;
    try {
        Matrix bad(2, 2, std::vector<float>(3, 1.0f)); // needs 4, given 3
        (void)bad;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

// operator() maps (row, col) to the right flat slot: index = row * cols + col.
static void test_flat_indexing() {
    Matrix M(2, 3, {0, 1, 2, 3, 4, 5});
    // Row-major: element value equals its flat index here.
    CHECK(M(0, 0) == 0.0f);
    CHECK(M(0, 2) == 2.0f);
    CHECK(M(1, 0) == 3.0f);
    CHECK(M(1, 2) == 5.0f);
    CHECK(M.data().size() == 6);
}

int main() {
    struct {
        const char* name;
        MultiplyFn fn;
    } variants[] = {
        {"naive", &Matrix::multiply_matrix_naive},
        {"accumulator", &Matrix::multiply_matrix_accumulator},
        {"transpose", &Matrix::multiply_matrix_transpose},
#if defined(__ARM_NEON)
        {"SIMD", &Matrix::multiply_matrix_SIMD},
#endif
    };

    for (const auto& v : variants) {
        std::printf("[multiply: %s]\n", v.name);
        test_all_ones(v.fn);
        test_identity(v.fn);
        test_known_2x2(v.fn);
        test_non_square(v.fn);
        test_inner_dim_seven(v.fn);
        test_dimension_mismatch_throws(v.fn);
    }

    std::printf("[class invariants]\n");
    test_constructor_validation();
    test_flat_indexing();

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    if (g_failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED\n");
    return 1;
}
