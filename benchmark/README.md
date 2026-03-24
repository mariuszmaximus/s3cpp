# Benchmark

Work in progress

This directory benchmarks multiple AWS S3 SDKs by lowering implementations to a tiny C ABI and loading them as shared libraries at runtime

## Prerequisite

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
./build/test
```

## Notes

- Benchmark infra is inspired by [@strager](https://www.github.com/strager) work [Faster than Rust and C++: the PERFECT hash table](https://www.youtube.com/watch?v=DMQ_HcNSOAI) video ([code](https://github.com/strager/perfect-hash-tables))
