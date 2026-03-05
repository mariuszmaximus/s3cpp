#ifndef S3CPP_AUTH
#define S3CPP_AUTH

#include "s3cpp/httpclient.h"
#include <string>

namespace s3cpp {

class AWSSigV4Signer {
public:
  AWSSigV4Signer(const std::string &access, const std::string &secret)
      : access_key(std::move(access)), secret_key(std::move(secret)),
        aws_region("us-east-2") {}
  AWSSigV4Signer(const std::string &access, const std::string &secret,
                 const std::string &region)
      : access_key(std::move(access)), aws_region(std::move(region)),
        secret_key(std::move(secret)) {}

  template <typename T> void sign(HttpRequestBase<T> &request);

  template <typename T>
  std::string createCannonicalRequest(
      HttpRequestBase<T> &request,
      const std::string &payload_hash =
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

  const unsigned char *sha256(const std::string &str);
  const unsigned char *HMAC_SHA256(const unsigned char *key, size_t key_len,
                                   const std::string &data);
  std::string hex(const unsigned char *hash);
  std::string url_encode(const std::string &value);
  std::string getTimestamp();
  [[nodiscard]] std::string getRegion() { return aws_region; }

private:
  std::string access_key;
  std::string secret_key;
  std::string aws_region;

  const unsigned char *deriveSigningKey(const std::string request_date);
};
} // namespace s3cpp

#endif
