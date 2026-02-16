#include "gtest/gtest.h"
#include <s3cpp/s3.h>
#include <string>

TEST(AWS, AWSAnonymousCredsListBucket) {
    S3Client client("", "");
    std::expected<ListObjectsResult, Error> res = client.ListObjects("fast-ai-imageclas");
    if (!res)
        FAIL() << std::format("ListObjects request failed. Code={}, Message={}", res.error().Code, res.error().Message);
}
