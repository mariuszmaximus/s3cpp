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

static std::string do_get_object(bench::ClientHandle handle, const char *key) {
  auto client = reinterpret_cast<S3Client *>(handle);
  std::expected<std::string, Error> result = client->GetObject("bucket", key);
  if (!result) {
    std::println("fatal: s3cpp get_object {}", result.error().Message);
    std::exit(1);
  }
  return result.value();
}

// ABI
extern "C" bench::ClientHandle initClient(const char *access, const char *secret,
                                          const char *endpoint) noexcept {
  auto handle = initialize_client(access, secret, endpoint);
  return handle;
}

extern "C" const char *get_object(bench::ClientHandle handle,
                                  const char *key) noexcept {
  static thread_local std::string result = do_get_object(handle, key);
  return result.c_str();
}
