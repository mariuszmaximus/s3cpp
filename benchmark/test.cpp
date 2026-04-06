#include "implementations.h"
#include "tasks.h"
#include <cstring>
#include <print>

namespace s3b {

void preamble() {
  auto client = s3cpp::S3Client("minio_access", "minio_secret", "127.0.0.1:9000", s3cpp::S3AddressingStyle::PathStyle);

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


void test_free_client(implementation impl) {
  bench::ClientHandle handle = impl.init_client("minio_access", "minio_secret", "127.0.0.1:9000");
  impl.free_client(handle);
  std::println("  OK free_client");
}

void test_create_bucket(implementation impl, const std::string &bucket) {
  bench::ClientHandle handle = impl.handler;
  impl.create_bucket(handle, bucket.c_str());
  std::println("  OK create_bucket ({})", bucket);
}

void test_put_object(implementation impl, const std::string &bucket) {
  bench::ClientHandle handle = impl.handler;
  impl.put_object(handle, bucket.c_str(), "put-test-key", "put-test-value");
  std::println("  OK put_object");
}

void test_get_object(implementation impl) {
  // Reads "foo" seeded by the preamble in "test-bucket"
  bench::ClientHandle handle = impl.handler;
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

  // Check that endpoint is valid and setup a dummy bucket to run tests on
  s3b::check_endpoint("127.0.0.1:9000");
  s3b::clean_up();
  s3b::preamble();

  for (const auto &impl : implementations) {
    std::println("{}", impl.name);
    s3b::test_init_client(impl);
    s3b::test_free_client(impl);
    std::string bucket = s3b::bucket_name_for(impl.name);
    s3b::test_create_bucket(impl, bucket);
    s3b::test_put_object(impl, bucket);
    s3b::test_get_object(impl);
  }

  s3b::clean_up();

  std::println("Good to go!");
}
