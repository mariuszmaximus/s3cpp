#include "gtest/gtest.h"
#include <s3cpp/s3.h>
#include <string>

using namespace s3cpp;

class S3 : public ::testing::Test {
    // Setup a MinIO bucket with some contents already
protected:
    static void SetUpTestCase() {
        S3Client client = S3Client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

        // Create bucket "my-bucket"
        auto createBucketRes = client.CreateBucket("my-bucket");
        if (!createBucketRes && createBucketRes.error().Code != "BucketAlreadyOwnedByYou") { // Skip if already created
            return;
        }

        // Upload 1001 files
        if (client.ListObjects("my-bucket")->Contents.empty()) {
            for (int i = 1; i <= 1'001; i++) {
                const std::string key = std::format("path/to/file_{}.txt", i);
                const std::string body = std::format("This is test file number {}", i);
                auto putObjRes = client.PutObject("my-bucket", key, body);
                if (!putObjRes)
                    return;
            }
        }

        return;
    }

    static void TearDownTestSuite() {
        // Remove stale data that was created by the test-suite
        // so we can re-run the suite as many times as we want
        S3Client client = S3Client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

        // This buckets *are* empty, therefore there is no need for a paginator to
        // remove the elements inside each bucket if they fill in at some point in
        // the future, this will throw error
        std::vector<std::string> BucketsToDelete {
            "test-bucket-s3cpp",
            "test-bucket-location",
            "test-bucket-tags"
        };
        for (const std::string& bName : BucketsToDelete) {
            client.DeleteBucket(bName);
        }
    }
};

TEST_F(S3, ListObjectsBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    // Assuming the bucket has 1001 objects
    // Once we implement PutObject we will do this ourselves with s3cpp
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket");
    if (!res)
        GTEST_FAIL();
    EXPECT_EQ(res->Contents.size(), 1000);
}

TEST_F(S3, ListObjectsBucketNotExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::expected<ListObjectsResult, Error> res = client.ListObjects("Does-not-exist");
    if (res.has_value()) // We must return error
        GTEST_FAIL();
    Error error = res.error();
    // EXPECT_EQ(error.Code, "InvalidBucketName");
}

TEST_F(S3, ListObjectsFilePrefix) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    // path/to/file_1.txt must exist
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .Prefix = "path/to/file_1.txt" });
    if (!res)
        GTEST_FAIL();
    EXPECT_EQ(res->Contents.size(), 1);
}

TEST_F(S3, ListObjectsDirPrefix) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    // Get 100 keys
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 100, .Prefix = "path/to/" });
    if (!res)
        GTEST_FAIL();
    EXPECT_EQ(res->Contents.size(), 100);
}

TEST_F(S3, ListObjectsDirPrefixMaxKeys) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 1, .Prefix = "path/to/" });
    if (!res)
        GTEST_FAIL();
    EXPECT_EQ(res->Contents.size(), 1);
}

TEST_F(S3, ListObjectsCheckFields) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 2, .Prefix = "path/to/" });
    if (!res)
        GTEST_FAIL();

    EXPECT_EQ(res->Name, "my-bucket");
    EXPECT_EQ(res->Prefix, "path/to/");
    EXPECT_EQ(res->MaxKeys, 2);
    EXPECT_EQ(res->IsTruncated, true);
    EXPECT_FALSE(res->NextContinuationToken.empty());

    // Should have exactly 2 keys
    EXPECT_EQ(res->Contents.size(), 2);

    EXPECT_EQ(res->Contents[0].Key, "path/to/file_1.txt");
    EXPECT_EQ(res->Contents[0].Size, 26);
    EXPECT_EQ(res->Contents[0].StorageClass, "STANDARD");

    EXPECT_EQ(res->Contents[1].Key, "path/to/file_10.txt");
    EXPECT_EQ(res->Contents[1].Size, 27);
    EXPECT_EQ(res->Contents[1].StorageClass, "STANDARD");
}

TEST_F(S3, ListObjectsCheckLenKeys) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    // has 1001 objects - limit is 1000 keys
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .Prefix = "path/to/" });
    if (!res)
        GTEST_FAIL();
    EXPECT_EQ(res->Contents.size(), 1000);
}

TEST_F(S3, ListObjectsPaginator) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    // has 1001 objects - fetch 100 per page
    ListObjectsPaginator paginator(client, "my-bucket", "path/to/", 100);

    int totalObjects = 0;
    int pageCount = 0;

    while (paginator.HasMorePages()) {
        std::expected<ListObjectsResult, Error> page = paginator.NextPage();
        if (!page) {
            GTEST_FAIL();
        }
        totalObjects += page->Contents.size();
        if (page->Contents.size() > 0)
            pageCount++;

        if (paginator.HasMorePages()) {
            EXPECT_EQ(page->Contents.size(), 100);
            EXPECT_TRUE(page->IsTruncated);
        }
    }

    EXPECT_EQ(totalObjects, 1001);
    EXPECT_EQ(pageCount, 11);
}

TEST_F(S3, GetObjectExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    auto response = client.GetObject("my-bucket", "path/to/file_1.txt");
    if (!response) {
        GTEST_FAIL();
    }
}

TEST_F(S3, GetObjectNotExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    auto response = client.GetObject("my-bucket", "does/not/exists.txt");
    if (response) { // should trigger error
        GTEST_FAIL();
    }
    EXPECT_EQ(response.error().Code, "NoSuchKey");
}

TEST_F(S3, GetObjectBadBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    auto response = client.GetObject("does-not-exist", "path/to/file_1.txt");
    if (response) { // should trigger error
        GTEST_FAIL();
    }
    EXPECT_EQ(response.error().Code, "NoSuchBucket");
}

TEST_F(S3, ListObjectsWithDelimiter) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .Delimiter = "/", .Prefix = "path/" });
    if (!res) {
        GTEST_FAIL();
    }

    // With delimiter, we should get CommonPrefixes for subdirectories
    EXPECT_FALSE(res->CommonPrefixes.empty());
    EXPECT_EQ(res->Delimiter, "/");
}

TEST_F(S3, ListObjectsWithStartAfter) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::expected<ListObjectsResult, Error> res = client.ListObjects("my-bucket", { .MaxKeys = 10, .Prefix = "path/to/", .StartAfter = "path/to/file_50.txt" });
    if (!res) {
        GTEST_FAIL();
    }

    // First key should be file_50.txt
    EXPECT_FALSE(res->Contents.empty());
    EXPECT_GT(res->Contents[0].Key, "path/to/file_50.txt");
}

TEST_F(S3, ListObjectsWithContinuationToken) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::expected<ListObjectsResult, Error> firstPage = client.ListObjects("my-bucket", { .MaxKeys = 10, .Prefix = "path/to/" });
    if (!firstPage) {
        GTEST_FAIL();
    }

    EXPECT_TRUE(firstPage->IsTruncated);
    EXPECT_FALSE(firstPage->NextContinuationToken.empty());

    // using continuation token
    std::expected<ListObjectsResult, Error> secondPage = client.ListObjects("my-bucket", { .ContinuationToken = firstPage->NextContinuationToken, .MaxKeys = 10, .Prefix = "path/to/" });
    if (!secondPage)
        GTEST_FAIL();

    EXPECT_FALSE(secondPage->Contents.empty());
    EXPECT_NE(firstPage->Contents[0].Key, secondPage->Contents[0].Key);
}

TEST_F(S3, GetObjectWithRange) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    // first 10 bytes
    auto response = client.GetObject("my-bucket", "path/to/file_1.txt", { .Range = "bytes=0-3" });
    if (!response) {
        GTEST_FAIL();
    }

    EXPECT_EQ(response.value().size(), 4);
}

TEST_F(S3, PutObjectTxt) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    auto PutResponse = client.PutObject("my-bucket", "some/file.txt", "hello, from s3");
    if (!PutResponse) {
        FAIL() << std::format("PutObject request failed: Code={}, Message={}",
            PutResponse.error().Code,
            PutResponse.error().Message);
    }
    std::expected<std::string, Error> GetResponse = client.GetObject("my-bucket", "some/file.txt");
    if (!GetResponse) {
        FAIL() << std::format("GetObject request failed: Code={}, Message={}",
            GetResponse.error().Code,
            GetResponse.error().Message);
    }
    EXPECT_EQ(GetResponse.value(), "hello, from s3");
}

TEST_F(S3, CreateBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::string bucketName = "test-bucket-s3cpp";
    CreateBucketConfiguration config;
    CreateBucketInput options;

    auto res = client.CreateBucket(bucketName, config, options);
    if (!res) {
        FAIL() << std::format("CreateBucket request failed: Code={}, Message={}",
            res.error().Code,
            res.error().Message);
    }

    std::expected<ListObjectsResult, Error> listRes = client.ListObjects(bucketName);
    if (!listRes) {
        FAIL() << std::format("ListObjects after CreateBucket failed: Code={}, Message={}",
            listRes.error().Code,
            listRes.error().Message);
    }
    EXPECT_EQ(listRes->Name, bucketName);
    EXPECT_EQ(listRes->Contents.size(), 0);
}

TEST_F(S3, CreateBucketWithLocationConstraint) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::string bucketName = "test-bucket-location";
    CreateBucketConfiguration config;
    config.LocationConstraint = "us-west-2";
    CreateBucketInput options;

    auto res = client.CreateBucket(bucketName, config, options);
    if (!res) {
        FAIL() << std::format("CreateBucket with LocationConstraint failed: Code={}, Message={}",
            res.error().Code,
            res.error().Message);
    }

    EXPECT_FALSE(res->Location.empty());
    EXPECT_EQ(res->Location, std::format("/{}", bucketName));

    std::expected<ListObjectsResult, Error> listRes = client.ListObjects(bucketName);
    if (!listRes) {
        FAIL() << "Failed to list objects in newly created bucket";
    }
    EXPECT_EQ(listRes->Name, bucketName);
}

TEST_F(S3, CreateBucketWithTags) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    std::string bucketName = "test-bucket-tags";
    CreateBucketConfiguration config;
    config.Tags = {
        { "Environment", "Test" },
        { "Project", "s3cpp" }
    };
    CreateBucketInput options;

    auto res = client.CreateBucket(bucketName, config, options);
    if (!res) {
        FAIL() << std::format("CreateBucket with Tags failed: Code={}, Message={}",
            res.error().Code,
            res.error().Message);
    }

    std::expected<ListObjectsResult, Error> listRes = client.ListObjects(bucketName);
    if (!listRes) {
        FAIL() << "Failed to list objects in newly created bucket";
    }
    EXPECT_EQ(listRes->Name, bucketName);
}

TEST_F(S3, CreateBucketAlreadyExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    CreateBucketConfiguration config;
    CreateBucketInput options;

    auto res = client.CreateBucket("my-bucket", config, options);
    if (res) {
        FAIL() << "CreateBucket should fail when bucket already exists";
    }

    // BucketAlreadyOwnedByYou error
    Error error = res.error();
    EXPECT_TRUE(error.Code == "BucketAlreadyOwnedByYou" || error.Code == "BucketAlreadyExists");
}

TEST_F(S3, DeleteBucketWithElements) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

    // Create bucket
    CreateBucketConfiguration config;
    CreateBucketInput options;
    auto createBucketRes = client.CreateBucket("bucket123", config, options);
    if (!createBucketRes && createBucketRes.error().Code != "BucketAlreadyOwnedByYou")
        FAIL() << "Unable to create bucket";

    // Add some dummy elements
    auto putObjectRes = client.PutObject("bucket123", "/path/to/file", "Body contents");
    // TODO(cristian): overload print operator for errors
    if (!putObjectRes)
        FAIL() << std::format("Unable to put object: Code={}, Message={}", putObjectRes.error().Code, putObjectRes.error().Message);
    ;

    // Delete must return error
    auto deleteBucketRes = client.DeleteBucket("bucket123");
    if (deleteBucketRes.has_value() || deleteBucketRes.error().Code != "BucketNotEmpty")
        FAIL() << "DeleteBucket should return 'Code=BucketNotEmpty'";
}

TEST_F(S3, DeleteEmptyBucket) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);
    CreateBucketConfiguration config;
    CreateBucketInput options;

    auto createBucketRes = client.CreateBucket("test-empty-bucket", config, options);
    if (!createBucketRes && createBucketRes.error().Code != "BucketAlreadyOwnedByYou") {
        FAIL() << std::format("DeleteEmptyBucket: Unable to create bucket: Code={}, Message={}",
            createBucketRes.error().Code,
            createBucketRes.error().Message);
    }

    // TODO(cristian): Implement overloading of = for client.* calls ???
    auto deleteBucket = client.DeleteBucket("test-empty-bucket");
    if (!deleteBucket) {
        FAIL() << std::format("DeleteBucket failed: Code={}, Message={}",
            deleteBucket.error().Code,
            deleteBucket.error().Message);
    }
}

TEST_F(S3, DeleteBucketAndElementsWithPaginator) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

    // Create bucket
    CreateBucketConfiguration config;
    CreateBucketInput options;
    auto createBucketRes = client.CreateBucket("test-bucket-321", config, options);
    if (!createBucketRes && createBucketRes.error().Code != "BucketAlreadyOwnedByYou")
        FAIL() << "Unable to create bucket";

    // Add some dummy elements
    for (int i = 0; i < 10; i++) {
        auto putObjectRes = client.PutObject("test-bucket-321", std::format("path/to/file{}", i + 1), "Body contents");
        if (!putObjectRes)
            FAIL() << std::format("Unable to put object: Code={}, Message={}", putObjectRes.error().Code, putObjectRes.error().Message);
    }

    // Remove each object with the Paginator
    ListObjectsPaginator paginator(client, "test-bucket-321", "", 4);
    while (paginator.HasMorePages()) {
        std::expected<ListObjectsResult, Error> page = paginator.NextPage();
        if (!page) {
            GTEST_FAIL();
        }
        for (const auto& obj : page->Contents) {
            auto req = client.DeleteObject("test-bucket-321", obj.Key);
            if (!req)
                FAIL() << std::format("Unable to delete object. Code={}, Message={}", req.error().Code, req.error().Message);
        }
    }

    // Delete must succeed now
    auto deleteBucketRes = client.DeleteBucket("test-bucket-321");
    if (!deleteBucketRes)
        FAIL() << std::format("Unable to delete bucket. Code={}, Message={}", deleteBucketRes.error().Code, deleteBucketRes.error().Message);
}

TEST_F(S3, HeadBucketExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

    // Premise is that "my-bucket" already exists and contains 1001 objects
    auto res = client.HeadBucket("my-bucket");
    if (!res)
        FAIL() << std::format("HeadBucket request failed for an existing bucket. Code={}, Message={}", res.error().Code, res.error().Message);
}

TEST_F(S3, HeadBucketNotExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

    auto res = client.HeadBucket("jhshdjksfjabhfndfds");
    if (res.has_value())
        FAIL() << std::format("HeadBucket request succeeded when it should have failed. Code={}, Message={}", res.error().Code, res.error().Message);

    EXPECT_EQ(res.error().Code, "NoSuchBucket");
}

TEST_F(S3, HeadObjectExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

    // Premise is that "my-bucket" already exists and contains "path/to/file_1.txt"
    auto res = client.HeadObject("my-bucket", "path/to/file_1.txt");
    if (!res)
        FAIL() << std::format("HeadObject request failed for an existing object. Code={}, Message={}", res.error().Code, res.error().Message);

    EXPECT_FALSE(res->ETag.empty());
    EXPECT_GT(res->ContentLength, 0);
    EXPECT_FALSE(res->LastModified.empty());
}

TEST_F(S3, HeadObjectNotExists) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

    auto res = client.HeadObject("my-bucket", "does/not/exist/file.txt");
    if (res.has_value())
        FAIL() << "HeadObject request succeeded when it should have failed for non-existent object";

    EXPECT_EQ(res.error().Code, "NoSuchKey");
}

TEST_F(S3, ListBuckets) {
    S3Client client("minio_access", "minio_secret", "127.0.0.1:9000", S3AddressingStyle::PathStyle);

    std::expected<ListAllMyBucketsResult, Error> res = client.ListBuckets();
    if (res) {
        EXPECT_EQ(res->Buckets.size(), 5);
        std::vector<std::string> bucket_names;
        for (const auto& Bucket : res->Buckets) {
            bucket_names.push_back(Bucket.Name);
        }
        EXPECT_TRUE(std::find(bucket_names.begin(), bucket_names.end(), "my-bucket") != bucket_names.end());
    } else {
        FAIL() << std::format("ListBuckets request failed. Code={}, Message={}", res.error().Code, res.error().Message);
    }
}
