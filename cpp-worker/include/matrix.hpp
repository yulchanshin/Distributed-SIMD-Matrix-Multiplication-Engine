#pragma once
#include <cstddef> //for std::size_t
#include <stdexcept>
#include <vector>

class Matrix {
private:
    std::size_t rows_;
    std::size_t cols_;
    std::vector<float> data_;

public:
    // constructor 1
    Matrix(std::size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        data_ = std::vector<float>(rows * cols, 0.0f);
    }

    // constructor 2
    Matrix(std::size_t rows, size_t cols, std::vector<float> data)
        : rows_(rows), cols_(cols) {
        if (data.size() != rows * cols) {
            throw std::invalid_argument("data size doesn't match rows * cols");
        }
        data_ = std::move(data);
    }

    // operator overloading for the 2D feel
    inline float& operator()(std::size_t row, size_t col) {
        return data_[row * cols_ + col];
    }

    // read only access
    inline float operator()(std::size_t row, size_t col) const {
        return data_[row * cols_ + col];
    }

    // matrix multiplication
    static Matrix multiply_matrix_naive(const Matrix& A, const Matrix& B) {
        if (A.cols() != B.rows()) {
            throw std::invalid_argument(
                "The column of Matrix A does not match the Row of Matrix B");
        }

        Matrix C(A.rows(), B.cols());

        for (std::size_t i{}; i < A.rows(); ++i) {
            for (std::size_t j{}; j < B.cols(); ++j) {
                for (std::size_t k{}; k < A.cols(); ++k)
                    C(i, j) += A(i, k) * B(k, j);
            }
        }

        return C;
    }

    // same O(N^3) algorithm, but accumulate each dot product in a local
    // (register) and write to C exactly once per cell. Avoids the repeated
    // RAM read/write of C(i,j) that aliasing can force the compiler to keep.
    static Matrix multiply_matrix_accumulator(const Matrix& A,
                                              const Matrix& B) {
        if (A.cols() != B.rows()) {
            throw std::invalid_argument(
                "The column of Matrix A does not match the Row of Matrix B");
        }

        Matrix C(A.rows(), B.cols());

        for (std::size_t i{}; i < A.rows(); ++i) {
            for (std::size_t j{}; j < B.cols(); ++j) {
                float sum = 0.0f;
                for (std::size_t k{}; k < A.cols(); ++k) {
                    sum += A(i, k) * B(k, j);
                }
                C(i, j) = sum;
            }
        }

        return C;
    }

    // getters
    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }
    const std::vector<float>& data() const { return data_; }
};
