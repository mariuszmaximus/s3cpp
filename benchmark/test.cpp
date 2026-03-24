#include "implementations.h"
#include <s3cpp/s3.h>
#include "tasks.h"

namespace s3b {

void preamble(const std::string &bucket, const std::string &key, const std::string &value) {
  auto bucketExists = [](s3cpp::S3Client &c, const std::string b) -> bool {
    auto result = c.HeadBucket(b);
    return result.has_value();
  };
  // Push a dummy bucket with some text to test that all SO can read
  auto client = s3cpp::S3Client("minio_access", "minio_secret", "127.0.0.1:9000", s3cpp::S3AddressingStyle::PathStyle);
  if (!bucketExists(client, bucket)) {
    auto result = client.CreateBucket("bucket");
    if (!result && result.error().Code != "BucketAlreadyOwnedByYou") {
      std::println("fatal(s3cpp): unable to create dummy bucket to test: {}",
                   result.error().Message);
      std::exit(1);
    }
  }
  auto keyExists = [](s3cpp::S3Client &c, std::string b, std::string k) -> bool {
    auto result = c.HeadObject(b, k);
    return result.has_value();
  };
  // PUT {"foo": "bar"}
  if (!keyExists(client, bucket, key)) {
    auto result = client.PutObject("bucket", "foo", "bar");
    if (!result) {
      std::println("fatal(s3cpp): unable to put object: {}", result.error().Message);
      std::exit(1);
    }
  }
}

void test_get_object(const char *name, implementation impl) {
  bench::ClientHandle handle = impl.initClient("minio_access", "minio_secret", "127.0.0.1:9000");
  const char *contents = impl.get_object(handle, "foo");
  if (strcmp(contents, "bar") != 0) {
      std::println("fatal: {} failed the test for get_object", impl.name);
      std::exit(1);
  }
}

} // namespace s3b

int main() {
  std::array<s3b::implementation, s3b::implementations_count> implementations = s3b::parse_implementations();

  s3b::preamble("bucket", "foo", "bar");

  for (const auto &implementation : implementations) {
    std::println("{}", implementation.name);
    s3b::test_get_object(implementation.name, implementation);
  }

  std::println("Good to go!");
}
