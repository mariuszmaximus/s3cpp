#include "s3cpp/auth.h"
#include "s3cpp/httpclient.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <format>
#include <iomanip>
#include <map>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <type_traits>

namespace s3cpp {

template <typename T> void AWSSigV4Signer::sign(HttpRequestBase<T> &request) {
  // For anonymous requests (empty credentials), only add required headers but
  // skip Authorization
  const bool is_anonymous = access_key.empty() || secret_key.empty();

  // Compute payload hash and set header for all requests
  std::string payload_hash;
  if constexpr (std::is_same_v<T, HttpBodyRequest>) {
    const std::string &body = static_cast<HttpBodyRequest &>(request).getBody();
    payload_hash = hex(sha256(body));
  } else {
    // Empty payload hash for GET/HEAD requests
    payload_hash =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
  }
  request.header("x-amz-content-sha256", payload_hash);
  sign(request, payload_hash);
}

template <typename T>
void AWSSigV4Signer::sign(HttpRequestBase<T> &request,
                          const std::string &payload_hash) {
  request.header("x-amz-content-sha256", payload_hash);

  // Skip signing for anonymous requests
  const bool is_anonymous = access_key.empty() || secret_key.empty();
  if (is_anonymous) {
    return;
  }

  // Autorization
  const std::string hash_algo = "AWS4-HMAC-SHA256";

  // Compute date and add it as a header
  const std::string timestamp = this->getTimestamp();
  request.header("X-Amz-Date", timestamp);
  const std::string request_date = timestamp.substr(0, 8);

  // Credential
  const std::string credential_scope =
      std::format("{}/{}/s3/aws4_request", request_date, aws_region);

  // Signed headers
  std::string signed_headers = "";
  uint_fast16_t i = 0;
  for (const auto &header : request.getHeaders()) {
    std::string kHeader = header.first;
    std::transform(kHeader.begin(), kHeader.end(), kHeader.begin(), ::tolower);
    if (i > 0)
      signed_headers += ";";
    signed_headers += kHeader;
    i++;
  }

  // Cannonical request
  std::string hex_cannonical_request =
      hex(sha256(createCannonicalRequest(request, payload_hash)));

  // To sign
  std::string string_to_sign =
      std::format("{}\n{}\n{}\n{}", hash_algo, timestamp, credential_scope,
                  hex_cannonical_request);
  std::string signature = hex(HMAC_SHA256(
      deriveSigningKey(request_date), SHA256_DIGEST_LENGTH, string_to_sign));

  // Build the final auth header value
  request.header(
      "Authorization",
      std::format("{} Credential={}/{}, SignedHeaders={}, Signature={}",
                  hash_algo, access_key, credential_scope, signed_headers,
                  signature));
}

template <typename T>
std::string
AWSSigV4Signer::createCannonicalRequest(HttpRequestBase<T> &request,
                                        const std::string &payload_hash) {
  const std::string http_verb =
      request.getHttpMethodStr(request.getHttpMethod());
  std::string url = request.getURL();

  // URI
  const size_t scheme_end = url.find("://");
  const size_t authority_start =
      scheme_end == std::string::npos ? 0 : scheme_end + 3;
  const size_t path_start = url.find_first_of("/?", authority_start);

  std::string uri;
  if (path_start == std::string::npos) {
    uri = "/";
  } else if (url[path_start] == '?') {
    uri = "/" + url.substr(path_start);
  } else {
    uri = url.substr(path_start);
  }
  size_t begin_q = uri.find("?");
  const std::string cannonical_uri =
      (begin_q != std::string::npos) ? uri.substr(0, begin_q) : uri;

  // URI Query-string
  std::map<std::string, std::string> query_params;
  while (begin_q != std::string::npos) {
    uri = uri.substr(begin_q + 1);
    begin_q = uri.find("&");
    const std::string query_param = uri.substr(0, begin_q);
    // Split query params by '=' character
    // Key=Value -> [Key, Value]
    const int equalPos = query_param.find('=');
    auto q = std::pair<std::string, std::string>(
        query_param.substr(0, equalPos), query_param.substr(equalPos + 1));
    query_params[q.first] = q.second;
  }

  // Insert alphabetically-sorted query params on the Cannonical Request
  std::string query_str = "";
  for (const auto &[key, value] : query_params) {
    if (query_str.size() > 0) {
      query_str += "&";
    }
    query_str += std::format("{}={}", url_encode(key), url_encode(value));
  }

  // Canonical Headers + SignedHeaders
  const std::map<std::string, std::string, LowerCaseCompare> headers =
      request.getHeaders();
  std::string cheaders = "";
  std::string signed_headers = "";
  uint_fast16_t i = 0;
  for (const auto &header : headers) {
    std::string kHeader = header.first;
    std::transform(kHeader.begin(), kHeader.end(), kHeader.begin(), ::tolower);
    if (i > 0)
      signed_headers += ";";
    signed_headers += kHeader;
    cheaders += kHeader + ":" + header.second + "\n";
    i++;
  }

  std::string canonical_request =
      std::format("{}\n{}\n{}\n{}\n{}\n{}", http_verb, cannonical_uri,
                  query_str, cheaders, signed_headers, payload_hash);

  return canonical_request;
}

const unsigned char *AWSSigV4Signer::sha256(const std::string &str) {
  static unsigned char digest[SHA256_DIGEST_LENGTH];
  const auto in_str = reinterpret_cast<const unsigned char *>(str.c_str());
  SHA256(in_str, str.size(), digest);
  return digest;
}

const unsigned char *AWSSigV4Signer::HMAC_SHA256(const unsigned char *key,
                                                 size_t key_len,
                                                 const std::string &data) {
  static unsigned char digest[SHA256_DIGEST_LENGTH];
  HMAC(EVP_sha256(), key, key_len,
       reinterpret_cast<const unsigned char *>(data.c_str()), data.size(),
       digest, NULL);
  return digest;
}

std::string AWSSigV4Signer::hex(const unsigned char *hash) {
  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(hash[i]);
  }
  return ss.str();
}

std::string AWSSigV4Signer::url_encode(const std::string &value) {
  std::string encoded;
  encoded.reserve(value.size() * 3);

  for (unsigned char c : value) {
    // RFC 3986
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
        c == '~') {
      encoded += c;
    } else {
      encoded += std::format("%{:02X}", c);
    }
  }
  return encoded;
}

// Required by AWS SigV4 to be in ISO8601 format
// https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_sigv-create-signed-request.html
std::string AWSSigV4Signer::getTimestamp() {
  const std::chrono::time_point now{std::chrono::system_clock::now()};
  return std::format("{:%Y%m%dT%H%M%SZ}",
                     std::chrono::floor<std::chrono::seconds>(now));
}

const unsigned char *
AWSSigV4Signer::deriveSigningKey(const std::string request_date) {
  const std::string initial_candidate = "AWS4" + secret_key;
  const unsigned char *keyCandidate =
      reinterpret_cast<const unsigned char *>(initial_candidate.c_str());

  unsigned char DateKey[SHA256_DIGEST_LENGTH];
  const unsigned char *temp =
      HMAC_SHA256(keyCandidate, initial_candidate.size(), request_date);
  std::memcpy(DateKey, temp, SHA256_DIGEST_LENGTH);

  unsigned char DateRegionKey[SHA256_DIGEST_LENGTH];
  temp = HMAC_SHA256(DateKey, SHA256_DIGEST_LENGTH, aws_region);
  std::memcpy(DateRegionKey, temp, SHA256_DIGEST_LENGTH);

  unsigned char DateRegionServiceKey[SHA256_DIGEST_LENGTH];
  temp = HMAC_SHA256(DateRegionKey, SHA256_DIGEST_LENGTH, "s3");
  std::memcpy(DateRegionServiceKey, temp, SHA256_DIGEST_LENGTH);

  return HMAC_SHA256(DateRegionServiceKey, SHA256_DIGEST_LENGTH,
                     "aws4_request");
}

// Why are we still here? Just to suffer?
template void AWSSigV4Signer::sign<HttpRequest>(HttpRequestBase<HttpRequest> &);
template void
AWSSigV4Signer::sign<HttpBodyRequest>(HttpRequestBase<HttpBodyRequest> &);
template void
AWSSigV4Signer::sign<HttpFileRequest>(HttpRequestBase<HttpFileRequest> &,
                                      const std::string &);
template std::string AWSSigV4Signer::createCannonicalRequest<HttpRequest>(
    HttpRequestBase<HttpRequest> &, const std::string &);
template std::string AWSSigV4Signer::createCannonicalRequest<HttpBodyRequest>(
    HttpRequestBase<HttpBodyRequest> &, const std::string &);
template std::string AWSSigV4Signer::createCannonicalRequest<HttpFileRequest>(
    HttpRequestBase<HttpFileRequest> &, const std::string &);

} // namespace s3cpp
