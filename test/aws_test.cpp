#include "gtest/gtest.h"
#include <expected>
#include <s3cpp/s3.h>
#include <string>

using namespace s3cpp;

TEST(AWS, AWSAnonymousCredsListBucket) {
    S3Client client("", "");
    std::expected<ListObjectsResult, Error> res = client.ListObjects("fast-ai-imageclas");
    if (!res)
        FAIL() << std::format("ListObjects request failed. Code={}, Message={}", res.error().Code, res.error().Message);

    EXPECT_EQ(res->Name, "fast-ai-imageclas");
    EXPECT_EQ(res->Prefix, "");
    EXPECT_EQ(res->MaxKeys, 1000);
    EXPECT_EQ(res->IsTruncated, false);
    EXPECT_TRUE(res->NextContinuationToken.empty());

    EXPECT_EQ(res->Contents.size(), 26);
    EXPECT_EQ(res->Contents[0].Key, "CUB_200_2011.tgz");
    EXPECT_EQ(res->Contents[0].Size, 1150585339);
    EXPECT_EQ(res->Contents[0].StorageClass, "STANDARD");

    EXPECT_EQ(res->Contents[1].Key, "bedroom.tgz");
    EXPECT_EQ(res->Contents[1].Size, 4579163978);
    EXPECT_EQ(res->Contents[1].StorageClass, "STANDARD");
}

