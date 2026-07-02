#pragma once

#include <grpcpp/grpcpp.h>
#include "matrix.grpc.pb.h"

class MatrixServiceImpl final : public matrix_engine::MatrixService::Service {
    grpc::Status Multiply(grpc::ServerContext* context,
                          const matrix_engine::MultiplyRequest* request,
                          matrix_engine::MultiplyResponse* response) override;   // <- just a semicolon, no body
};
