#include "../tasks.h"
#include <print>
#include <s3cpp/s3.h>
#include <string>

using namespace s3cpp;

static bench::ClientHandle initialize_client(const char *access,
                                             const char *secret,
                                             const char *endpoint) {
  auto client = new S3Client(access, secret,
                             endpoint, // 127.0.0.1:9000
                             s3cpp::S3AddressingStyle::PathStyle);
  return reinterpret_cast<bench::ClientHandle>(client);
}

static void do_create_bucket(bench::ClientHandle handle, const char *bucket) {
  auto client = reinterpret_cast<S3Client *>(handle);
  auto result = client->CreateBucket(bucket);
  if (!result) {
    std::println("fatal: s3cpp create_bucket {}", result.error().Message);
    std::exit(1);
  }
  return;
}

static void do_put_object(bench::ClientHandle handle, const char *bucket,
                          const char *key, const char *contents) {
  auto client = reinterpret_cast<S3Client *>(handle);
  auto result = client->PutObject(bucket, key, contents);
  if (!result) {
    std::println("fatal: s3cpp put_object {}", result.error().Message);
    std::exit(1);
  }
  return;
}

static std::string do_get_object(bench::ClientHandle handle, const char *bucket, const char *key) {
  auto client = reinterpret_cast<S3Client *>(handle);
  std::expected<std::string, Error> result = client->GetObject(bucket, key);
  if (!result) {
    std::println("fatal: s3cpp get_object {}", result.error().Message);
    std::exit(1);
  }
  return result.value();
}

// ABI
extern "C" bench::ClientHandle init_client(const char *access,
                                           const char *secret,
                                           const char *endpoint) noexcept {
  auto handle = initialize_client(access, secret, endpoint);
  return handle;
}

extern "C" void create_bucket(bench::ClientHandle handle,
                              const char *bucket) noexcept {
  do_create_bucket(handle, bucket);
  return;
}

extern "C" void put_object(bench::ClientHandle handle,
                                  const char *bucket, const char *key,
                                  const char *contents) noexcept {
  do_put_object(handle, bucket, key, contents);
  return;
}

extern "C" const char *get_object(bench::ClientHandle handle,
                                  const char *bucket, const char *key) noexcept {
  thread_local std::string result;
  result = do_get_object(handle, bucket, key);
  return result.c_str();
}

extern "C" void free_client(bench::ClientHandle handle) noexcept {
    delete reinterpret_cast<S3Client *>(handle);
}
