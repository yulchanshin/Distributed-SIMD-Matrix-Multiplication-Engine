#pragma once
#include <cstddef> //for std::size_t
#include <stdexcept>
#include <vector>
#include <arm_neon.h>

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
    inline const float& operator()(std::size_t row, size_t col) const {
        return data_[row * cols_ + col];
    }
    
    //O(N^2) 
    Matrix transpose() const {
        Matrix B_T(cols_, rows_);
        for (std::size_t i = 0; i < rows_; ++i){
            for (std::size_t j = 0; j < cols_; ++j){
                B_T(j, i) = (*this)(i,j);
            }
        }
        return B_T;
    }

    static Matrix multiply_matrix_SIMD(const Matrix& A, const Matrix& B){
        if (A.cols() != B.rows()){
            throw std::invalid_argument(
                "The column of Matrix A does not match the row of Matrix B");
        }

        Matrix C(A.rows(), B.cols());
        //transpose to avoid cache thrashing!
        Matrix B_T = B.transpose();

        for (std::size_t i = 0; i < A.rows(); ++i){
            for (std::size_t j = 0; j < B_T.rows(); ++j){
              float32x4_t sum_vec = vdupq_n_f32(0.0f);
              std::size_t k = 0;
              for (; k + 4 <= B_T.cols(); k += 4){
                  float32x4_t a_vec = vld1q_f32(&A(i, k));
                  float32x4_t b_vec = vld1q_f32(&B_T(j, k));

                  sum_vec = vmlaq_f32(sum_vec, a_vec, b_vec);
              }
              float final_sum_arr[4];
              vst1q_f32(final_sum_arr, sum_vec);
              float final_dot_prod = final_sum_arr[0] + final_sum_arr[1] +
                                     final_sum_arr[2] + final_sum_arr[3];

              for(; k < B_T.cols(); k++){
                  final_dot_prod += A(i, k) * B_T(j,k);
              }

              C(i,j) = final_dot_prod;
            }
        }
        return C;
    }

    static Matrix multiply_matrix_transpose(const Matrix& A, const Matrix& B){
        if (A.cols() != B.rows()){
            throw std::invalid_argument(
                "The column of Matrix A does not match the row of Matrix B");
        }

        Matrix C(A.rows(), B.cols());
        //transpose to avoid cache thrashing!
        Matrix B_T = B.transpose();

        for (size_t i = 0; i < A.rows(); ++i){
            for (size_t j = 0; j < B_T.rows(); ++j){
              float sum = 0.0f;
              for (size_t k = 0; k < B_T.cols(); ++k){
                  sum += A(i, k) * B_T(j, k);
              }
              C(i, j) = sum;
            }
        }
        return C;
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
