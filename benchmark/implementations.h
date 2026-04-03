#ifndef S3CPP_BENCHMARK_IMPLEMENTATIONS_H
#define S3CPP_BENCHMARK_IMPLEMENTATIONS_H

#include "tasks.h"
#include <cstddef>
#include <cstdio>
#include <dlfcn.h>
#include <iterator>
#include <print>
#include <vector>

namespace s3b {

constexpr const char *implementations_so[] = {
#include "build/implementations.inc"
};

constexpr std::size_t implementations_count = std::size(implementations_so);

struct implementation {
  const char *name;
  bench::init_client_f *init_client;
  bench::create_bucket_f *create_bucket;
  bench::put_object_f *put_object;
  bench::get_object_f *get_object;
};

inline implementation parse_implementation(const char *so_path) {
  void *so_handle = dlopen(so_path, RTLD_LAZY);
  if (so_handle == NULL) {
    std::println(stderr, "fatal: unable to get handle {}", so_path);
    std::exit(1);
  }

  std::string symbol_name{};

  symbol_name = "init_client";
  bench::init_client_f *init_client = reinterpret_cast<bench::init_client_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (init_client == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  symbol_name = "create_bucket";
  bench::create_bucket_f *create_bucket = reinterpret_cast<bench::create_bucket_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (create_bucket == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  symbol_name = "put_object";
  bench::put_object_f *put_object = reinterpret_cast<bench::put_object_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (put_object == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  symbol_name = "get_object";
  bench::get_object_f *get_object = reinterpret_cast<bench::get_object_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (get_object == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }
  return implementation{so_path, init_client, create_bucket, put_object, get_object};
}

inline std::array<implementation, implementations_count>
parse_implementations() {
  std::array<implementation, implementations_count> implementations{};
  for (size_t i = 0; i < implementations_count; i++) {
    implementations[i] = parse_implementation(implementations_so[i]);
  }
  return implementations;
}

} // namespace s3b

#endif
