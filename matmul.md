# Engineering Project Specification & Agile Roadmap
## Title: Distributed SIMD Matrix Multiplication Engine (Go + Redis + gRPC + C++20)

# Jira: yulhanshin.atlassian.net

---

## 1. Project Overview & High-Level Architecture

This project is a production-grade distributed system designed to demonstrate advanced concepts in low-level hardware optimization, memory management, and decoupled network architecture. It is split into two primary layers:

1.  **The Control Plane (Go):** Functions as a high-throughput API Gateway. It manages user connection pools, exposes developer-facing endpoints, runs cryptographic hashing on inputs, and handles ultra-fast in-memory caching via Redis.
2.  **The Data Plane (C++20):** Works as a dedicated, bare-metal compute engine. It communicates exclusively via binary serialization using gRPC over HTTP/2. The inner compute loops are custom-tuned to fit CPU cache lines (L1/L2 data caches) and utilize single-instruction, multiple-data (SIMD) hardware registers.

### Directory Structure Blueprint
Establish this exact repository structure to handle the multi-language workspace cleanly:

```text
├── .github/
│   └── workflows/
│       └── ci.yml             # GitHub Actions CI Automation
├── go-gateway/
│   ├── main.go                # API Router and Core Logic
│   ├── cache/                 # Redis Client Wrapper
│   └── proto/                 # Generated Go gRPC Bindings
├── cpp-worker/
│   ├── CMakeLists.txt         # Root C++ Build File
│   ├── include/               # Header Files
│   ├── src/
│   │   ├── main.cpp           # C++ gRPC Server Entrypoint
│   │   └── matrix.cpp         # Custom SIMD Math Logic
│   └── benchmarks/
│       └── benchmark_main.cpp # Google Benchmark Suites
├── proto/
│   └── matrix.proto           # Protobuf Protocol Contract
└── docker-compose.yml         # Container Orchestration Spec
```

---

## Progress Tracker

Tick a box when its ticket is merged into `main`.

**Week 1 — Bare-Metal Compute Engine**
- [X] TSK-101: Build System Setup & Google Benchmark Integration
- [X] TSK-102: Naive Matrix Multiplication & Flat Memory Class
- [ ] TSK-103: Cache Locality Optimization (Matrix Transposition)
- [ ] TSK-104: Hardware-Level Vectorization via AVX2 Intrinsics

**Week 2 — Network Bridge**
- [ ] TSK-201: Protocol Contract Schema Definition
- [ ] TSK-202: Native C++ gRPC Service Execution

**Week 3 — Distributed Control Plane**
- [ ] TSK-301: Go API Gateway Web Router Setup
- [ ] TSK-302: Cryptographic Cache Layer via Redis
- [ ] TSK-303: Multi-Container Docker Compose Environment

**Week 4 — Terminal UI Dashboard & CI**
- [ ] TSK-401: Terminal UI Live Telemetry Dashboard
- [ ] TSK-402: Automated Build and Performance CI Pipeline

---

## 2. Global Branching & Contribution Strategy

To simulate a professional environment, protect your `main` branch. Every task on your Kanban board maps to an isolated git branch. Follow this execution loop for every ticket:

1.  **Create Branch:** `git checkout -b feature/TSK-XXX-short-description`
2.  **Write Code & Verify:** Build features locally and run tests/benchmarks.
3.  **Atomic Commit:** `git commit -m "feat(scope): descriptive commit message detailing changes"`
4.  **Push & Review:** `git push origin feature/TSK-XXX-short-description`
5.  **Merge Rule:** Open a Pull Request on GitHub, self-review the diff, and merge into `main` only when all compilation checks pass.

---

## 3. Detailed Weekly Tickets & Implementation Guide

### WEEK 1: The Bare-Metal Compute Engine (C++20 & Math Optimization)
*Focus: Single-machine performance boundaries, memory architecture layouts, and micro-benchmarking.*

#### TSK-101: Build System Setup & Google Benchmark Integration
* **Git Branch:** `feature/tsk-101-cmake-setup`
* **Description:** Initialize the `cpp-worker/` directory and configure modern CMake targeting C++20. Use CMake's `FetchContent` module to dynamically download, compile, and link the Google Benchmark library without relying on system-wide binary installations. Configure strong build profile optimizations globally.
* **Acceptance Criteria:**
    * Running `cmake -B build -DCMAKE_BUILD_TYPE=Release` completes without errors.
    * A sample test benchmark compiles and successfully runs, outputting timing statistics in the terminal.
    * Compiler optimization flags `-O3` and `-march=native` are explicitly declared in the build outputs.

#### TSK-102: Naive Matrix Multiplication & Flat Memory Class
* **Git Branch:** `feature/tsk-102-naive-matrix`
* **Description:** Avoid memory fragmentation caused by nested wrappers like `std::vector<std::vector<float>>`. Implement a custom `Matrix` class storing data in a single contiguous flat 1D array (`std::vector<float> data(rows * cols)`). Write standard 2D-to-1D index translation logic: `index = (row * cols) + col`. Write a classic, un-optimized triple-nested loop ($O(N^3)$) to perform matrix multiplication. Register this execution loop inside your Google Benchmark suite.
* **Acceptance Criteria:**
    * The matrix class correctly computes known matrix multiplication test cases.
    * Google Benchmark outputs baseline nanosecond profiles for dimensions: 256x256, 512x512, and 1024x1024.

#### TSK-103: Cache Locality Optimization (Matrix Transposition)
* **Git Branch:** `feature/tsk-103-cache-transpose`
* **Description:** The naive loop reads the second matrix down columns, breaking cache lines and flooding the system with L1 cache misses. Write an optimized multiplication path that creates a temporary transposed copy of Matrix B before executing the calculation. This adjustment ensures the inner loop reads both arrays sequentially row-by-row, staying completely within ultra-fast CPU caches.
* **Acceptance Criteria:**
    * Profile the execution using Valgrind/Cachegrind (`valgrind --tool=cachegrind ./build/bin/matrix_benchmark`).
    * Verify a significant drop in L1 Data Cache read miss percentage.
    * Google Benchmark must report at least a 2x to 4x speedup over the baseline loop for a 1024x1024 matrix.

#### TSK-104: Hardware-Level Vectorization via AVX2 Intrinsics
* **Git Branch:** `feature/tsk-104-simd-avx2`
* **Description:** Include `<immintrin.h>`. Rewrite the core math calculation loop utilizing Intel/AMD AVX2 vector intrinsics. Use 256-bit registers to load 8 single-precision floating-point numbers simultaneously via `_mm256_loadu_ps`. Multiply them in parallel with `_mm256_mul_ps` and accumulate them into vector accumulators. Create a clean fallback loop to correctly handle matrix edge dimensions not divisible by 8.
* **Acceptance Criteria:**
    * The SIMD engine passes strict unit testing matching the math accuracy of the naive baseline.
    * The execution timeline shows massive speed improvements over the un-vectorized loop.

---

### WEEK 2: The Network Bridge (gRPC & Protocol Buffers)
*Focus: Polyglot interface integration and ultra-low latency internal data serialization.*

#### TSK-201: Protocol Contract Schema Definition
* **Git Branch:** `feature/tsk-201-protobuf-schema`
* **Description:** Create a language-neutral network schema inside `proto/matrix.proto`. Define a strict typed message format for `Matrix` (containing rows, columns, and a repeated flat float array). Create `MultiplyRequest` and `MultiplyResponse` wrappers. Declare a `MatrixService` possessing an RPC function named `Multiply`.
* **Acceptance Criteria:**
    * The `.proto` file strictly complies with `proto3` syntax guidelines.
    * Running the protocol compiler (`protoc`) compiles the definition into valid C++ header/source files and native Go structures without type warnings.

#### TSK-202: Native C++ gRPC Service Execution
* **Git Branch:** `feature/tsk-202-cpp-grpc-server`
* **Description:** Link gRPC dependencies into your C++ workspace. Inherit from the generated protobuf base service class. Implement the server interface by binding the incoming network payload arrays straight to your Phase 1 AVX2 optimized math functions. Instantiate a `grpc::ServerBuilder` to launch a thread-safe server listening for raw binary calls on internal port `50051`.
* **Acceptance Criteria:**
    * The C++ gRPC server binary compiles without error.
    * The server boots up locally and actively listens for network traffic on port 50051.

---

### WEEK 3: Distributed Control Plane (Go, Redis, & Containerization)
*Focus: Concurrent networking, caching mechanics, and container isolation orchestration.*

#### TSK-301: Go API Gateway Web Router Setup
* **Git Branch:** `feature/tsk-301-go-gateway`
* **Description:** Initialize a Go project module inside `go-gateway/`. Implement a web routing framework (such as Go Fiber or native `net/http`). Expose a public POST endpoint `/api/v1/compute`. Write payload validation logic to ensure that Matrix A's column count matches Matrix B's row count before allowing any network routing to continue.
* **Acceptance Criteria:**
    * Sending a malformed matrix layout returns an HTTP 400 bad request error.
    * Sending a correctly formatted JSON payload maps the matrix data cleanly into local memory models.

#### TSK-302: Cryptographic Cache Layer via Redis
* **Git Branch:** `feature/tsk-302-redis-caching`
* **Description:** To prevent wasting CPU cycles on repetitive mathematical requests, write a utility function that serializes incoming matrix arrays into a byte buffer and calculates a unique **SHA-256 hash key**. Integrate a Go Redis client wrapper. On a request, check Redis using this hash key.
    * *Cache Hit:* Return the result matrix directly from memory cache, bypassing gRPC completely.
    * *Cache Miss:* Route the request via gRPC to the C++ server, receive the result, write it to Redis with a 24-hour TTL, and respond to the client.
* **Acceptance Criteria:**
    * The first math request triggers a cache miss and calls the C++ worker.
    * An identical follow-up request executes as a cache hit, returning results in under 5 milliseconds.

#### TSK-303: Multi-Container Docker Compose Environment
* **Git Branch:** `feature/tsk-303-docker-orchestration`
* **Description:** Containerize the system. Build a multi-stage Dockerfile for Go to ensure minimal image sizes. Build an isolated Dockerfile for C++ that compiles CMake dependencies and sets up the gRPC environment. Write a root `docker-compose.yml` stringing together three independent services: `redis`, `cpp-worker`, and `go-gateway` on an isolated virtual network bridge.
* **Acceptance Criteria:**
    * Running `docker-compose up --build` compiles both codebases and provisions all services seamlessly.
    * Containers successfully discover and communicate with each other using Docker DNS names (`cpp-worker:50051`, `redis:6379`).

---

### WEEK 4: Terminal UI Dashboard & CI Automations (The Victory Lap)
*Focus: Telemetry instrumentation, terminal rendering, and continuous integration.*

#### TSK-401: Terminal UI Live Telemetry Dashboard
* **Git Branch:** `feature/tsk-401-tui-dashboard`
* **Description:** Create an asynchronous text-based monitoring engine using **Charmbracelet's Bubble Tea** framework in Go. The application acts as an administrative dashboard split into three clear terminal view areas:
    * *Metrics Panel:* Displays live system counters tracking total processing volume, active cache hit/miss ratio percentages, and network latency statistics.
    * *System Logs:* A real-time viewport streaming background engine events and structural shape dimensions of incoming computations.
* **Acceptance Criteria:**
    * The terminal dashboard runs smoothly without flickering or locking up the CLI display.
    * Firing benchmark requests at the Go API Gateway causes the TUI counters to dynamically update in real time.

#### TSK-402: Automated Build and Performance CI Pipeline
* **Git Branch:** `feature/tsk-402-ci-pipeline`
* **Description:** Author a complete automated pipeline configuration script inside `.github/workflows/ci.yml`. On every pull request or push to the `main` branch, spin up an isolated virtual container to install dependencies (`cmake`, `libbenchmark-dev`, `protobuf-compiler`), build your C++ codebase with high optimizations, and execute your performance verification checks.
* **Acceptance Criteria:**
    * Pushing code to GitHub triggers the Actions runner automatically.
    * The pipeline catches code compilation errors or failing test runs before allowing code merges into `main`.

---

## 4. Engineering Performance Logbook

Use this standardized measurement table to track your progress as you implement optimizations. Use these exact metrics to drive your final technical blog post analysis:

| Phase / Optimization Stage | Matrix Dimensions | Execution Speed (ms) | Relative Speedup | Core Bottleneck / Constraint |
| :--- | :--- | :--- | :--- | :--- |
| **Phase 1.2: Naive Matrix Loop** | 1024x1024 | 1028.95 | 1.0x (Baseline) | Memory Bandwidth / Cache Trashing |
| **Phase 1.3: Transposed Matrix** | 1024x1024 | 425.95 | 2.42x | CPU Scalar Limits |
| **Phase 1.4: SIMD AVX2 Active** | 1024x1024 | | | Core Clock Frequency Bounds |
| **Phase 3.2: Redis Cache Hit** | 1024x1024 | | | Network TCP I/O Limits |
