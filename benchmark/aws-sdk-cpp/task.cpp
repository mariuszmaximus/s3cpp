#include "../tasks.h"
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <print>
#include <sstream>
#include <string>

static bench::ClientHandle initialize_client(const char *access,
                                             const char *secret,
                                             const char *endpoint) {
  setenv("AWS_EC2_METADATA_DISABLED", "true", 1);
  Aws::SDKOptions options;
  Aws::InitAPI(options);

  Aws::S3::S3ClientConfiguration clientConfig;
  clientConfig.endpointOverride = "http://127.0.0.1:9000/";
  clientConfig.region = "eu-west-1";
  clientConfig.scheme = Aws::Http::Scheme::HTTP;

  Aws::Auth::AWSCredentials credentials(access, secret);

  auto client = new Aws::S3::S3Client(
      credentials, clientConfig,
      Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
      false);
  return reinterpret_cast<bench::ClientHandle>(client);
}

static void do_create_bucket(bench::ClientHandle handle, const char *bucket) {
  auto client = reinterpret_cast<Aws::S3::S3Client *>(handle);

  Aws::S3::Model::CreateBucketRequest request;
  request.SetBucket(bucket);

  auto outcome = client->CreateBucket(request);
  if (!outcome.IsSuccess()) {
    std::println("fatal: aws-sdk-cpp create_bucket {}",
                 outcome.GetError().GetMessage());
    std::exit(1);
  }
}

static void do_put_object(bench::ClientHandle handle, const char *bucket,
                          const char *key, const char *contents) {
  auto client = reinterpret_cast<Aws::S3::S3Client *>(handle);

  Aws::S3::Model::PutObjectRequest request;
  request.SetBucket(bucket);
  request.SetKey(key);

  auto body = Aws::MakeShared<Aws::StringStream>("PutObject");
  *body << contents;
  request.SetBody(body);

  auto outcome = client->PutObject(request);
  if (!outcome.IsSuccess()) {
    std::println("fatal: aws-sdk-cpp put_object {}",
                 outcome.GetError().GetMessage());
    std::exit(1);
  }
}

static std::string do_get_object(bench::ClientHandle handle, const char *bucket, const char *key) {
  auto client = reinterpret_cast<Aws::S3::S3Client *>(handle);

  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(bucket);
  request.SetKey(key);

  auto outcome = client->GetObject(request);
  if (!outcome.IsSuccess()) {
    std::println("fatal: aws-sdk-cpp get_object {}",
                 outcome.GetError().GetMessage());
    std::exit(1);
  }

  std::ostringstream ss;
  ss << outcome.GetResult().GetBody().rdbuf();
  return ss.str();
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
}

extern "C" void put_object(bench::ClientHandle handle,
                           const char *bucket, const char *key,
                           const char *contents) noexcept {
  do_put_object(handle, bucket, key, contents);
}

extern "C" const char *get_object(bench::ClientHandle handle,
                                  const char *bucket, const char *key) noexcept {
  thread_local std::string result;
  result = do_get_object(handle, bucket, key);
  return result.c_str();
}

extern "C" void free_client(bench::ClientHandle handle) noexcept {
  delete reinterpret_cast<Aws::S3::S3Client *>(handle);
}
