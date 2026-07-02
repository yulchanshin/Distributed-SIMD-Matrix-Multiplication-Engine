#include "matrix_service.hpp"
#include "matrix.hpp" 

#include <vector>
#include <chrono>
// Out-of-line definition of the method declared in matrix_service.hpp.
grpc::Status MatrixServiceImpl::Multiply(grpc::ServerContext* context,
                                         const matrix_engine::MultiplyRequest* request,
                                         matrix_engine::MultiplyResponse* response) {
    std::vector<float> matrix_a_data(request->matrix_a().data().begin(), request->matrix_a().data().end());
    std::vector<float> matrix_b_data(request->matrix_b().data().begin(), request->matrix_b().data().end());

    Matrix A(request->matrix_a().rows(), request->matrix_a().cols(), matrix_a_data); 
    Matrix B(request->matrix_b().rows(), request->matrix_b().cols(), matrix_b_data);
    
    auto start = std::chrono::steady_clock::now();
    Matrix C = Matrix::multiply_matrix_SIMD(A, B);
    auto end = std::chrono::steady_clock::now();

    float elapsed_ms = std::chrono::duration<float, std::milli>(end - start).count();

    response->set_execution_time_ms(elapsed_ms);
    
    matrix_engine::Matrix* result = response->mutable_result_matrix();
    result->set_rows(C.rows());
    result->set_cols(C.cols());
    result->mutable_data()->Assign(C.data().begin(), C.data().end());

    return grpc::Status::OK;
}
