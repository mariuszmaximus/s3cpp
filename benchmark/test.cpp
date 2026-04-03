#include "implementations.h"
#include <s3cpp/s3.h>
#include "tasks.h"
#include <cstring>
#include <print>

namespace s3b {

void preamble() {
  auto client = s3cpp::S3Client("minio_access", "minio_secret", "127.0.0.1:9000", s3cpp::S3AddressingStyle::PathStyle);

  // Clean up stale buckets from previous runs
  auto buckets = client.ListBuckets();
  if (buckets) {
    for (const auto &b : buckets->Buckets) {
      // Delete all objects in the bucket first
      auto objects = client.ListObjects(b.Name);
      if (objects) {
        for (const auto &obj : objects->Contents) {
          client.DeleteObject(b.Name, obj.Key);
        }
      }
      client.DeleteBucket(b.Name);
    }
  }

  // Seed a bucket with test data for get_object tests
  auto bucket = client.CreateBucket("test-bucket");
  if (!bucket) {
    std::println("fatal(preamble): create_bucket {}", bucket.error().Message);
    std::exit(1);
  }

  auto put = client.PutObject("test-bucket", "foo", "bar");
  if (!put) {
    std::println("fatal(preamble): put_object {}", put.error().Message);
    std::exit(1);
  }
}

void test_init_client(implementation impl) {
  bench::ClientHandle handle = impl.init_client("minio_access", "minio_secret", "127.0.0.1:9000");
  if (handle == nullptr) {
    std::println("  FAIL init_client: returned nullptr");
    std::exit(1);
  }
  std::println("  OK init_client");
}

std::string bucket_name_for(const char *so_path) {
  std::string_view name = so_path;
  auto slash = name.rfind('/');
  if (slash != std::string_view::npos) name = name.substr(slash + 1);
  // strip lib prefix and .dylib/.so suffix
  if (name.starts_with("lib")) name.remove_prefix(3);
  if (name.ends_with(".dylib")) name.remove_suffix(6);
  else if (name.ends_with(".so")) name.remove_suffix(3);
  return "test-" + std::string(name);
}

void test_create_bucket(implementation impl, const std::string &bucket) {
  bench::ClientHandle handle = impl.init_client("minio_access", "minio_secret", "127.0.0.1:9000");
  impl.create_bucket(handle, bucket.c_str());
  std::println("  OK create_bucket ({})", bucket);
}

void test_put_object(implementation impl, const std::string &bucket) {
  bench::ClientHandle handle = impl.init_client("minio_access", "minio_secret", "127.0.0.1:9000");
  impl.put_object(handle, bucket.c_str(), "put-test-key", "put-test-value");
  std::println("  OK put_object");
}

void test_get_object(implementation impl) {
  // Reads "foo" seeded by the preamble in "test-bucket"
  bench::ClientHandle handle = impl.init_client("minio_access", "minio_secret", "127.0.0.1:9000");
  const char *contents = impl.get_object(handle, "test-bucket", "foo");
  if (std::strcmp(contents, "bar") != 0) {
    std::println("  FAIL get_object: expected \"bar\", got \"{}\"", contents);
    std::exit(1);
  }
  std::println("  OK get_object");
}

} // namespace s3b

int main() {
  std::array<s3b::implementation, s3b::implementations_count> implementations = s3b::parse_implementations();

  s3b::preamble();

  for (const auto &impl : implementations) {
    std::println("{}", impl.name);
    s3b::test_init_client(impl);
    std::string bucket = s3b::bucket_name_for(impl.name);
    s3b::test_create_bucket(impl, bucket);
    s3b::test_put_object(impl, bucket);
    s3b::test_get_object(impl);
  }

  std::println("Good to go!");
}
