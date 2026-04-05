#ifndef S3CPP_BENCHMARK_IMPLEMENTATIONS_H
#define S3CPP_BENCHMARK_IMPLEMENTATIONS_H

#include "s3cpp/s3.h"
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
  bench::ClientHandle handler;
  // Benchmark ABI
  bench::init_client_f *init_client;
  bench::create_bucket_f *create_bucket;
  bench::put_object_f *put_object;
  bench::get_object_f *get_object;
  bench::free_client_f *free_client;
};

inline implementation parse_implementation(const char *so_path) {
  void *so_handle = dlopen(so_path, RTLD_LAZY);
  if (so_handle == NULL) {
    std::println(stderr, "fatal: unable to get handle {}", so_path);
    std::exit(1);
  }

  std::string symbol_name{};

  symbol_name = "init_client";
  bench::init_client_f *init_client =
      reinterpret_cast<bench::init_client_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (init_client == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  symbol_name = "create_bucket";
  bench::create_bucket_f *create_bucket =
      reinterpret_cast<bench::create_bucket_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (create_bucket == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  symbol_name = "put_object";
  bench::put_object_f *put_object =
      reinterpret_cast<bench::put_object_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (put_object == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  symbol_name = "get_object";
  bench::get_object_f *get_object =
      reinterpret_cast<bench::get_object_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (get_object == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  symbol_name = "free_client";
  bench::free_client_f *free_client =
      reinterpret_cast<bench::free_client_f *>(dlsym(so_handle, symbol_name.c_str()));
  if (free_client == NULL) {
    std::println(stderr, "fatal: {} for {}", ::dlerror(), so_path);
    std::exit(1);
  }

  return implementation{so_path,     init_client("minio_access", "minio_secret", "127.0.0.1:9000"),
                        init_client, create_bucket,
                        put_object,  get_object, free_client};
}

inline std::array<implementation, implementations_count> parse_implementations() {
  std::array<implementation, implementations_count> implementations{};
  for (size_t i = 0; i < implementations_count; i++) {
    implementations[i] = parse_implementation(implementations_so[i]);
  }
  return implementations;
}

inline std::string bucket_name_for(const char *so_path) {
  std::string_view name = so_path;
  auto slash = name.rfind('/');
  if (slash != std::string_view::npos)
    name = name.substr(slash + 1);
  // strip lib prefix and .dylib/.so suffix
  if (name.starts_with("lib"))
    name.remove_prefix(3);
  if (name.ends_with(".dylib"))
    name.remove_suffix(6);
  else if (name.ends_with(".so"))
    name.remove_suffix(3);
  return "test-" + std::string(name);
}

inline void remove_objects(s3cpp::S3Client &client, const std::string &bucket) {
  s3cpp::ListObjectsPaginator paginator(client, bucket, "", 1000);
  while (paginator.HasMorePages()) {
    auto page = paginator.NextPage();
    if (!page) {
      std::println("Error listing objects: {}", page.error().Message);
      exit(1);
    }
    for (const auto &obj : page->Contents) {
      auto result = client.DeleteObject(bucket, obj.Key);
      if (!result) {
        std::println("Error deleting {}: {}", obj.Key, result.error().Message);
        exit(1);
      }
    }
  }
}

inline void clean_up() {
  // Wipe out everything inside out endpoint MinIO container
  // just to start the benchmark with a clean S3 instance

  // TODO(cristian): use another port/ip for benchmark to not collide with core s3cpp tests...
  s3cpp::S3Client client =
      s3cpp::S3Client("minio_access", "minio_secret", "127.0.0.1:9000", s3cpp::S3AddressingStyle::PathStyle);

  auto result = client.ListBuckets();
  if (!result.has_value()) {
    std::println("fatal(cleanup): unable to list buckets. {}", result.error().Message);
    exit(1);
  }
  auto buckets = std::move(*result);
  for (const auto &bucket : buckets.Buckets) {
    remove_objects(client, bucket.Name);

    // all objects gone, remove bucket
    auto result = client.DeleteBucket(bucket.Name);
    if (!result) {
      std::println("Error deleting bucket: {}", result.error().Message);
      exit(1);
    }
  }
}

inline void check_endpoint(const std::string &endpoint) {
  if (!s3cpp::Ping(endpoint)) {
    std::println(stderr, "MinIO is not running. Start it with:\n"
                         "  docker build -t s3cpp-minio:latest ..\n"
                         "  docker run -d -p 9000:9000 -p 9001:9001 \\\n"
                         "    -e \"MINIO_ROOT_USER=minio_access\" \\\n"
                         "    -e \"MINIO_ROOT_PASSWORD=minio_secret\" \\\n"
                         "    s3cpp-minio:latest \\\n"
                         "    server /data --console-address \":9001\"");
    std::exit(1);
  }
}

} // namespace s3b

#endif
