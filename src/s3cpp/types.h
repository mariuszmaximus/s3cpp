#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace s3cpp {

enum class S3AddressingStyle {
    VirtualHosted,
    PathStyle
};

// ListObjects
// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjectsV2.html#API_ListObjectsV2_ResponseSyntax
struct ListObjectsInput {
    std::optional<std::string> ContinuationToken;
    std::optional<std::string> Delimiter;
    std::optional<std::string> EncodingType;
    std::optional<std::string> ExpectedBucketOwner;
    std::optional<bool> FetchOwner;
    std::optional<int> MaxKeys;
    std::optional<std::string> Prefix;
    std::optional<std::string> RequestPayer;
    std::optional<std::string> StartAfter;
};

struct Contents_ {
    std::string ChecksumAlgorithm;
    std::string ChecksumType;
    std::string ETag;
    std::string Key;
    std::string LastModified;
    struct Owner_ {
        std::string DisplayName;
        std::string ID;
    } Owner;
    struct RestoreStatus_ {
        bool IsRestoreInProgress;
        std::string RestoreExpiryDate;
    } RestoreStatus;
    int64_t Size;
    std::string StorageClass;
};

struct CommonPrefix {
    std::string Prefix;
};

struct GetObjectInput {
    std::optional<std::string> If_Match;
    std::optional<std::string> If_Modified_Since;
    std::optional<std::string> If_None_Match;
    std::optional<std::string> If_Unmodified_Since;
    std::optional<int> partNumber;
    std::optional<std::string> Range; // e.g. bytes=0-9
    std::optional<std::string> response_cache_control;
    std::optional<std::string> response_content_disposition;
    std::optional<std::string> response_content_encoding;
    std::optional<std::string> response_content_language;
    std::optional<std::string> response_content_type;
    std::optional<std::string> response_expires;
    std::optional<std::string> versionId;
};

struct ListObjectsResult {
    bool IsTruncated;
    std::string Marker;
    std::string NextMarker;
    std::vector<Contents_> Contents;
    std::string Name;
    std::string Prefix;
    std::string Delimiter;
    int MaxKeys;
    std::vector<CommonPrefix> CommonPrefixes;
    std::string EncodingType;
    int KeyCount;
    std::string ContinuationToken;
    std::string NextContinuationToken;
    std::string StartAfter;
};

struct ListBucketsInput {
    std::optional<std::string> BucketRegion;
    std::optional<std::string> ContinuationToken;
    std::optional<int> MaxBuckets;
    std::optional<std::string> Prefix;
};

struct Bucket {
    std::string BucketARN;
    std::string BucketRegion;
    std::string CreationDate;
    std::string Name;
};

struct ListAllMyBucketsResult {
    std::vector<Bucket> Buckets;
    struct Owner_ {
        std::string DisplayName;
        std::string ID;
    } Owner;
    std::string ContinuationToken;
    std::string Prefix;
};

struct PutObjectInput {
    std::optional<std::string> CacheControl;
    std::optional<std::string> ContentDisposition;
    std::optional<std::string> ContentEncoding;
    std::optional<std::string> ContentLanguage;
    std::optional<int64_t> ContentLength;
    std::optional<std::string> ContentMD5;
    std::optional<std::string> ContentType;
    std::optional<std::string> Expires;
    std::optional<std::string> IfMatch;
    std::optional<std::string> IfNoneMatch;
    std::optional<std::string> ACL;
    std::optional<std::string> GrantFullControl;
    std::optional<std::string> GrantRead;
    std::optional<std::string> GrantReadACP;
    std::optional<std::string> GrantWriteACP;
    std::optional<std::string> ChecksumCRC32;
    std::optional<std::string> ChecksumCRC32C;
    std::optional<std::string> ChecksumCRC64NVME;
    std::optional<std::string> ChecksumSHA1;
    std::optional<std::string> ChecksumSHA256;
    std::optional<std::string> SDKChecksumAlgorithm;
    std::optional<std::string> ServerSideEncryption;
    std::optional<std::string> SSEKMSKeyId;
    std::optional<bool> SSEBucketKeyEnabled;
    std::optional<std::string> SSEKMSEncryptionContext;
    std::optional<std::string> SSECustomerAlgorithm;
    std::optional<std::string> SSECustomerKey;
    std::optional<std::string> SSECustomerKeyMD5;
    std::optional<std::string> ObjectLockLegalHold;
    std::optional<std::string> ObjectLockMode;
    std::optional<std::string> ObjectLockRetainUntilDate;
    std::optional<std::string> ExpectedBucketOwner;
    std::optional<std::string> RequestPayer;
    std::optional<std::string> StorageClass;
    std::optional<std::string> Tagging;
    std::optional<std::string> WebsiteRedirectLocation;
    std::optional<int64_t> WriteOffsetBytes;
};

struct PutObjectResult {
    std::string ETag;
    std::string Expiration;
    std::string ChecksumCRC32;
    std::string ChecksumCRC32C;
    std::string ChecksumCRC64NVME;
    std::string ChecksumSHA1;
    std::string ChecksumSHA256;
    std::string ChecksumType;
    std::string ServerSideEncryption;
    std::string VersionId;
    std::string SSECustomerAlgorithm;
    std::string SSECustomerKeyMD5;
    std::string SSEKMSKeyId;
    std::string SSEKMSEncryptionContext;
    bool BucketKeyEnabled;
    int64_t Size;
    std::string RequestCharged;
};

struct Tag {
    std::string Key;
    std::string Value;
};

struct CreateBucketConfiguration {
    struct BucketInfo {
        std::string DataRedundancy;
        std::string Type;
    } Bucket;
    struct LocationInfo {
        std::string Name;
        std::string Type;
    } Location;
    std::string LocationConstraint;
    std::vector<Tag> Tags;
};

struct CreateBucketInput {
    std::optional<std::string> ACL;
    std::optional<bool> ObjectLockEnabledForBucket;
    std::optional<std::string> GrantFullControl;
    std::optional<std::string> GrantRead;
    std::optional<std::string> GrantReadACP;
    std::optional<std::string> GrantWrite;
    std::optional<std::string> GrantWriteACP;
    std::optional<std::string> ObjectOwnership;
};

struct CreateBucketResult {
    std::string Location;
    std::optional<std::string> BucketARN;
};

struct DeleteObjectInput {
    std::optional<std::string> versionId;
    std::optional<std::string> If_Match;
    std::optional<std::string> If_MatchLastModifiedTime;
    std::optional<std::string> If_MatchSize;
    std::optional<std::string> MFA;
    std::optional<std::string> RequestPayer;
    std::optional<std::string> ByPassGovernanceRetention;
    std::optional<std::string> ExpectedBucketOwner;
};

struct DeleteObjectResult {
    std::string versionId;
    std::string RequestCharged;
    std::string DeleteMarker;
};

struct DeleteBucketInput {
    std::optional<std::string> ExpectedBucketOwner;
};

struct HeadBucketResult {
    std::string BucketARN;
    std::string BucketLocationType;
    std::string BucketLocationName;
    std::string BucketRegion;
    std::string AccessPointAlias;
};

struct HeadBucketInput {
    std::optional<std::string> ExpectedBucketOwner;
};

struct HeadObjectResult {
    bool DeleteMarker;
    std::string AcceptRanges;
    std::string Expiration;
    std::string Restore;
    std::string ArchiveStatus;
    std::string LastModified;
    int64_t ContentLength;
    std::string ChecksumCRC32;
    std::string ChecksumCRC32C;
    std::string ChecksumCRC64NVME;
    std::string ChecksumSHA1;
    std::string ChecksumSHA256;
    std::string ChecksumType;
    std::string ETag;
    int MissingMeta;
    std::string VersionId;
    std::string CacheControl;
    std::string ContentDisposition;
    std::string ContentEncoding;
    std::string ContentLanguage;
    std::string ContentType;
    std::string ContentRange;
    std::string Expires;
    std::string WebsiteRedirectLocation;
    std::string ServerSideEncryption;
    std::string SSECustomerAlgorithm;
    std::string SSECustomerKeyMD5;
    std::string SSEKMSKeyId;
    bool BucketKeyEnabled;
    std::string StorageClass;
    std::string RequestCharged;
    std::string ReplicationStatus;
    int PartsCount;
    int TagCount;
    std::string ObjectLockMode;
    std::string ObjectLockRetainUntilDate;
    std::string ObjectLockLegalHoldStatus;
};

struct HeadObjectInput {
    std::optional<std::string> If_Match;
    std::optional<std::string> If_Modified_Since;
    std::optional<std::string> If_None_Match;
    std::optional<std::string> If_Unmodified_Since;
    std::optional<int> partNumber;
    std::optional<std::string> Range; // e.g. bytes=0-9
    std::optional<std::string> response_cache_control;
    std::optional<std::string> response_content_disposition;
    std::optional<std::string> response_content_encoding;
    std::optional<std::string> response_content_language;
    std::optional<std::string> response_content_type;
    std::optional<std::string> response_expires;
    std::optional<std::string> versionId;
    std::optional<std::string> CheckSumMode;
    std::optional<std::string> ExpectedBucketOwner;
    std::optional<std::string> RequestPayer;
    std::optional<std::string> SideEncryptionCustomerAlgorithm;
    std::optional<std::string> SideEncryptionCustomerKey;
    std::optional<std::string> SideEncryptionCustomerKeyMD5;
};

// REST generic error
// https://docs.aws.amazon.com/AmazonS3/latest/API/ErrorResponses.html#RESTErrorResponses
struct Error {
    std::string Code;
    std::string Message;
    std::string Resource;
    std::string RequestId;
};

} // namespace s3cpp
