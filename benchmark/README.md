# Benchmark

This directory benchmarks multiple AWS S3 SDKs by lowering implementations to a tiny C ABI and loading them as shared libraries at runtime. Benchmarks are run using [Google Benchmark](https://github.com/google/benchmark) against a local MinIO instance.

All operations are sequential and single-threaded. No concurrent or parallel requests are measured.

### InitClient

Create a new S3 client from credentials and endpoint. The client is freed between each iteration.

| SDK | Time | CPU |
|---|---|---|
| [aws-sdk-go-v2](https://github.com/aws/aws-sdk-go-v2) | 19,169 ns | 18,482 ns |
| [aws-sdk-cpp](https://github.com/aws/aws-sdk-cpp) | 1,534,570 ns | 1,534,728 ns |
| [aws-sdk-rust](https://github.com/awslabs/aws-sdk-rust) | 136,442 ns | 136,707 ns |
| [s3cpp](https://github.com/ggcr/s3cpp) (ours) | 2,099 ns | 2,115 ns |

### CreateBucket

Create a new bucket each iteration. Client is already initialized.

| SDK | Time | CPU |
|---|---|---|
| [aws-sdk-go-v2](https://github.com/aws/aws-sdk-go-v2) | 2,065,452 ns | 141,830 ns |
| [aws-sdk-cpp](https://github.com/aws/aws-sdk-cpp) | 2,139,594 ns | 365,792 ns |
| [aws-sdk-rust](https://github.com/awslabs/aws-sdk-rust) | 2,018,148 ns | 241,299 ns |
| [s3cpp](https://github.com/ggcr/s3cpp) (ours) | 1,968,110 ns | 181,595 ns |

### PutObject

Upload an empty object to an existing bucket each iteration.

| SDK | Time | CPU |
|---|---|---|
| [aws-sdk-go-v2](https://github.com/aws/aws-sdk-go-v2) | 2,693,701 ns | 160,558 ns |
| [aws-sdk-cpp](https://github.com/aws/aws-sdk-cpp) | 3,100,104 ns | 540,917 ns |
| [aws-sdk-rust](https://github.com/awslabs/aws-sdk-rust) | 2,584,035 ns | 267,083 ns |
| [s3cpp](https://github.com/ggcr/s3cpp) (ours) | 2,577,575 ns | 178,125 ns |

### GetObject

Fetch a small object from an existing bucket each iteration.

| SDK | Time | CPU |
|---|---|---|
| [aws-sdk-go-v2](https://github.com/aws/aws-sdk-go-v2) | 867,896 ns | 172,226 ns |
| [aws-sdk-cpp](https://github.com/aws/aws-sdk-cpp) | 943,522 ns | 378,008 ns |
| [aws-sdk-rust](https://github.com/awslabs/aws-sdk-rust) | 828,040 ns | 270,482 ns |
| [s3cpp](https://github.com/ggcr/s3cpp) (ours) | 690,424 ns | 171,775 ns |

Ran on GitHub Actions CI ([full run](https://github.com/ggcr/s3cpp/actions/runs/24022463397/job/70053992177)) -- 2-core runner, results may vary.

## Prerequisite

```bash
brew install aws-sdk-cpp google-benchmark
```

A local MinIO instance is expected to be running already. The current `test.cpp` uses `s3cpp` itself as a preamble to create basic scaffolding for the tests before calling each shared library implementation

```bash
$ docker build -t s3cpp-minio:latest ..
$ docker run -d -p 9000:9000 -p 9001:9001 \
      -e "MINIO_ROOT_USER=minio_access" \
      -e "MINIO_ROOT_PASSWORD=minio_secret" \
      s3cpp-minio:latest \
      server /data --console-address ":9001"
```

## Usage

Build all currently enabled shared libraries:

```bash
make
```

Tests that shared libraries are built:
```bash
make test
```

Run full benchmark against all supported SDKs:
```bash
make bench
```

## Notes

- Benchmark infra is inspired by [@strager](https://www.github.com/strager) work [Faster than Rust and C++: the PERFECT hash table](https://www.youtube.com/watch?v=DMQ_HcNSOAI) video ([code](https://github.com/strager/perfect-hash-tables))
