# Benchmark

| Operation | aws-sdk-go-v2 | aws-sdk-cpp | aws-sdk-rust | s3cpp |
|---|---|---|---|---|
| InitClient | 0.02 ms | 1.53 ms | 0.14 ms | **0.002 ms** |
| CreateBucket | 2.07 ms | 2.14 ms | 2.02 ms | **1.97 ms** |
| PutObject | 2.69 ms | 3.10 ms | 2.58 ms | **2.58 ms** |
| GetObject | 0.87 ms | 0.94 ms | 0.83 ms | **0.69 ms** |

Ran on GitHub Actions CI ([full run](https://github.com/ggcr/s3cpp/actions/runs/24022463397/job/70053992177)) -- 2-core runner, results may vary.

---

Work in progress

This directory benchmarks multiple AWS S3 SDKs by lowering implementations to a tiny C ABI and loading them as shared libraries at runtime

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
