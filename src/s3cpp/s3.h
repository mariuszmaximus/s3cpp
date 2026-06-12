#ifndef S3CPP_S3
#define S3CPP_S3

#include <expected>
#include <functional>
#include <s3cpp/auth.h>
#include <s3cpp/httpclient.h>
#include <s3cpp/types.h>
#include <s3cpp/xml.hpp>

namespace s3cpp {

bool Ping(const std::string& endpoint_);

class S3Client {
public:
  S3Client(const std::string &access, const std::string &secret)
      : Client(HttpClient()), Signer(AWSSigV4Signer(access, secret)),
        Parser(XMLParser()),
        addressing_style_(S3AddressingStyle::VirtualHosted) {
    // When no endpoint is provided we default to us-east-1
    endpoint_ = std::format("s3.us-east-1.amazonaws.com");
  }
  S3Client(const std::string &access, const std::string &secret,
           const std::string &region)
      : Client(HttpClient()), Signer(AWSSigV4Signer(access, secret, region)),
        Parser(XMLParser()),
        addressing_style_(S3AddressingStyle::VirtualHosted) {
    // When no endpoint is provided we default to AWS
    endpoint_ =
        std::format("s3.{}.amazonaws.com", region); // TODO(cristian): Ping?
  }
  S3Client(const std::string &access, const std::string &secret,
           const std::string &customEndpoint, S3AddressingStyle style)
      : S3Client(access, secret, customEndpoint, style, false, "us-east-1") {}
  S3Client(const std::string &access, const std::string &secret,
           const std::string &customEndpoint, S3AddressingStyle style,
           bool useHttps, const std::string &region)
      : Client(HttpClient()), Signer(AWSSigV4Signer(access, secret, region)),
        Parser(XMLParser()), endpoint_(customEndpoint),
        addressing_style_(style), use_https_(useHttps) {}


  // S3 operations: Goal is to support CRUD and stay minimal
  std::expected<ListObjectsResult, Error> ListObjects(const std::string &bucket, const ListObjectsInput &options = {});
  std::expected<ListAllMyBucketsResult, Error> ListBuckets(const ListBucketsInput &options = {});
  std::expected<std::string, Error> GetObject(const std::string &bucket, const std::string &key, const GetObjectInput &options = {});
  std::expected<PutObjectResult, Error> PutObject(const std::string &bucket, const std::string &key, const std::string &body, const PutObjectInput &options = {});
  std::expected<PutObjectResult, Error>
  PutObjectFile(const std::string &bucket, const std::string &key,
                const std::string &filename,
                const std::string &contentType = "application/octet-stream",
                UploadProgressCallback progress = {});
  std::expected<DeleteObjectResult, Error> DeleteObject(const std::string &bucket, const std::string &key, const DeleteObjectInput &options = {});
  std::expected<CreateBucketResult, Error> CreateBucket(const std::string &bucket, const CreateBucketConfiguration &configuration = {}, const CreateBucketInput &options = {}); std::expected<void, Error> DeleteBucket(const std::string &bucket, const DeleteBucketInput &options = {});
  std::expected<HeadBucketResult, Error> HeadBucket(const std::string &bucket, const HeadBucketInput &options = {});
  std::expected<HeadObjectResult, Error> HeadObject(const std::string &bucket, const std::string &key, const HeadObjectInput &options = {});

  // TODO(cristian): Support adding .timeout() for each AWS op
  
  // XML serde
  std::expected<ListObjectsResult, Error> deserializeListObjectsResult(const std::vector<XMLNode> &nodes, const int maxKeys);
  std::expected<ListAllMyBucketsResult, Error> deserializeListBucketsResult(const std::vector<XMLNode> &nodes, std::optional<int> maxBuckets);
  std::expected<PutObjectResult, Error> deserializePutObjectResult( const std::map<std::string, std::string, LowerCaseCompare> &headers);
  std::expected<DeleteObjectResult, Error> deserializeDeleteObjectResult( const std::map<std::string, std::string, LowerCaseCompare> &headers);
  std::expected<CreateBucketResult, Error> deserializeCreateBucketResult( const std::map<std::string, std::string, LowerCaseCompare> &headers);
  std::expected<HeadBucketResult, Error> deserializeHeadBucketResult( const std::map<std::string, std::string, LowerCaseCompare> &headers);
  std::expected<HeadObjectResult, Error> deserializeHeadObjectResult( const std::map<std::string, std::string, LowerCaseCompare> &headers);

  Error deserializeError(const std::vector<XMLNode> &nodes);

private:
  HttpClient Client;
  AWSSigV4Signer Signer;
  XMLParser Parser;
  std::string endpoint_;
  S3AddressingStyle addressing_style_;
  bool use_https_ = true;

  std::string buildURL(const std::string &bucket) const {
    if (addressing_style_ == S3AddressingStyle::VirtualHosted) {
      // bucket.s3.region.amazonaws.com/key
      return std::format("https://{}.{}", bucket, endpoint_);
    } else {
      // endpoint/bucket/key
      return std::format("{}://{}/{}", use_https_ ? "https" : "http",
                         endpoint_, bucket);
    }
  }

  std::string getHostHeader(const std::string &bucket) const {
    if (addressing_style_ == S3AddressingStyle::VirtualHosted) {
      return std::format("{}.{}", bucket, endpoint_);
    } else {
      return endpoint_;
    }
  }
};

class ListObjectsPaginator {
public:
  ListObjectsPaginator(S3Client &client, const std::string &bucket,
                       const std::string &prefix)
      : client_(client), bucket_(bucket), prefix_(prefix), maxKeys_(1000) {}
  ListObjectsPaginator(S3Client &client, const std::string &bucket,
                       const std::string &prefix, int maxKeys)
      : client_(client), bucket_(bucket), prefix_(prefix), maxKeys_(maxKeys) {}

  bool HasMorePages() const { return hasMorePages_; }

  std::expected<ListObjectsResult, Error> NextPage() {
    ListObjectsInput options;
    if (!continuationToken_.empty())
      options.ContinuationToken = continuationToken_;
    if (!prefix_.empty())
      options.Prefix = prefix_;
    options.MaxKeys = maxKeys_;

    auto response = client_.ListObjects(bucket_, options);
    if (response.has_value()) {
      hasMorePages_ = response.value().IsTruncated;
      continuationToken_ = response.value().NextContinuationToken;
    }
    return response;
  }

private:
  S3Client &client_;
  std::string bucket_;
  std::string prefix_;
  int maxKeys_;
  bool hasMorePages_ = true;
  std::string continuationToken_;
};

} // namespace S3CPP_S3

#endif 
