#!/usr/bin/env bash
#
# Regenerate gRPC/protobuf stubs from proto/matrix.proto (TSK-201).
# Generated code is gitignored — run this after cloning or after editing the schema.
#
# Requires: protoc, grpc_cpp_plugin (brew install protobuf grpc),
#           protoc-gen-go, protoc-gen-go-grpc (go install ... ; needs $GOPATH/bin on PATH).

set -euo pipefail

# Run from the repo root regardless of where the script is invoked.
cd "$(dirname "$0")"

PROTO_DIR="proto"
CPP_OUT="cpp-worker/generated"
GO_OUT="go-gateway"

mkdir -p "$CPP_OUT" "$GO_OUT"

echo "==> C++ (messages + gRPC service) -> $CPP_OUT"
protoc -I "$PROTO_DIR" \
  --cpp_out="$CPP_OUT" \
  --grpc_out="$CPP_OUT" \
  --plugin=protoc-gen-grpc="$(which grpc_cpp_plugin)" \
  "$PROTO_DIR/matrix.proto"

echo "==> Go (structs + gRPC) -> $GO_OUT/matrixpb"
protoc -I "$PROTO_DIR" \
  --go_out="$GO_OUT" \
  --go-grpc_out="$GO_OUT" \
  "$PROTO_DIR/matrix.proto"

echo "==> Done."
