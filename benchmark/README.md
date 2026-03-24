# Benchmark

This directory benchmarks multiple S3 client implementations by lowering them all to a shared C ABI and loading them as shared libraries at runtime

The idea is:

- each implementation exports the same symbols, currently `initClient` and `get_object`
- each implementation is compiled into its own shared library in `build/`
- the host test/benchmark code loads those libraries with `dlopen`/`dlsym`
- once everything looks the same at the ABI boundary, the host can exercise all implementations uniformly

This keeps the benchmark harness language-agnostic. The host does not need to know whether the underlying implementation was written in C++, Go, or something else. It only cares that the shared library exports the expected symbols with the expected ABI.

## Prerequisite

A local MinIO instance is expected to be running already. The current `test.cpp` uses `s3cpp` itself as a preamble to create basic scaffolding for the tests before calling each shared library implementation.

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

- Benchmark infra is inspired by @strager work [Faster than Rust and C++: the PERFECT hash table](https://www.youtube.com/watch?v=DMQ_HcNSOAI) video ([code](https://github.com/strager/perfect-hash-tables))
