#include <grpcpp/grpcpp.h>
#include "matrix.grpc.pb.h"
#include "matrix_service.hpp"
#include <string>
#include <memory>
#include <iostream>

int main() {
    std::string server_address("0.0.0.0:50051");
    MatrixServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
    return 0;
}
