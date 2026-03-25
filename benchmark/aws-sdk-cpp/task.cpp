#include "../tasks.h"
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <print>
#include <sstream>
#include <string>

static bench::ClientHandle initialize_client(const char *access,
                                             const char *secret,
                                             const char *endpoint) {
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

static std::string do_get_object(bench::ClientHandle handle, const char *key) {
  auto client = reinterpret_cast<Aws::S3::S3Client *>(handle);

  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket("bucket");
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
extern "C" bench::ClientHandle initClient(const char *access,
                                          const char *secret,
                                          const char *endpoint) noexcept {
  auto handle = initialize_client(access, secret, endpoint);
  return handle;
}

extern "C" const char *get_object(bench::ClientHandle handle,
                                  const char *key) noexcept {
  static thread_local std::string result = do_get_object(handle, key);
  return result.c_str();
}
