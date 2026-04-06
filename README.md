# s3cpp

A lightweight C++ client library for AWS S3 with zero deps (only libcurl and OpenSSL)

For a performance comparison against other AWS SDKs on a small set of sequential tasks, see [benchmark/README.md](benchmark/README.md)

## Architecture

Each S3 Client is organized onto modular components:

- `src/s3cpp/httpclient`
- `src/s3cpp/auth`: AWS Signature V4 auth protocol
- `src/s3cpp/xml`: A custom FSM for parsing valid XML

## Basic Usage

Create a bucket:

```cpp
#include <s3cpp/s3.h>

int main() {
    s3cpp::S3Client client("access_key", "secret_key");

    auto result = client.CreateBucket("my-bucket", {
        .LocationConstraint = "us-east-1"
    });

    if (!result) {
        std::println("Error: {}", result.error().Message);
        return 1;
    }
    return 0;
}
```

List all buckets:

```cpp
#include <s3cpp/s3.h>

int main() {
    s3cpp::S3Client client("access_key", "secret_key");

    auto result = client.ListBuckets();

    if (!result) {
        std::println("Error: {}", result.error().Message);
        return 1;
    }

    for (const auto& bucket : result->Buckets) {
        std::println("Bucket: {}, Created: {}", bucket.Name, bucket.CreationDate);
    }
    return 0;
}
```

List objects in a bucket:

```cpp
#include <s3cpp/s3.h>

int main() {
    s3cpp::S3Client client("access_key", "secret_key");

    // List 100 objects with a prefix
    auto result = client.ListObjects("my-bucket", {
        .MaxKeys = 100,
        .Prefix = "path/to/"
    });

    if (!result) {
        std::println("Error: {}", result.error().Message);
        return 1;
    }

    for (const auto& obj : result->Contents) {
        std::println("Key: {}, Size: {}", obj.Key, obj.Size);
    }
    return 0;
}
```

For buckets with many objects, use the paginator to automatically handle continuation tokens:

```cpp
#include <s3cpp/s3.h>

int main() {
    s3cpp::S3Client client("access_key", "secret_key");
    s3cpp::ListObjectsPaginator paginator(client, "my-bucket", "path/to/", 100);

    int totalObjects = 0;

    while (paginator.HasMorePages()) {
        std::expected<s3cpp::ListObjectsResult, s3cpp::Error> page = paginator.NextPage();

        if (!page) {
            std::println("Error: {}", page.error().Message);
            return 1;
        }

        totalObjects += page->KeyCount;

        for (const auto& obj : page->Contents) {
            std::println("Key: {}", obj.Key);
        }
    }
    return 0;
}
```

Checking if a bucket exists: 

```cpp
#include <s3cpp/s3.h>

bool BucketExists(s3cpp::S3Client& client, const std::string& bucketName) {
    auto result = client.HeadBucket(bucketName);
    return result.has_value();
}

int main() {
    s3cpp::S3Client client("access_key", "secret_key");
    
    if (BucketExists(client, "my-bucket")) {
        std::println("Bucket exists");
    } else {
        std::println("Bucket does not exist");
    }
    
    return 0;
}
```

Delete a non-empty bucket:

```cpp
#include <s3cpp/s3.h>

int main() {
    s3cpp::S3Client client("access_key", "secret_key");

    // To delete a bucket we first need to delete all its contents
    s3cpp::ListObjectsPaginator paginator(client, "my-bucket", "", 1000);

    while (paginator.HasMorePages()) {
        auto page = paginator.NextPage();

        if (!page) {
            std::println("Error listing objects: {}", page.error().Message);
            return 1;
        }

        for (const auto& obj : page->Contents) {
            auto result = client.DeleteObject("my-bucket", obj.Key);
            if (!result) {
                std::println("Error deleting {}: {}", obj.Key, result.error().Message);
                return 1;
            }
        }
    }

    auto result = client.DeleteBucket("my-bucket");
    if (!result) {
        std::println("Error deleting bucket: {}", result.error().Message);
        return 1;
    }

    std::println("Bucket deleted successfully");
    return 0;
}
```

## Build and Test

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/tests
```

Some tests require a local MinIO container to be running:

```bash
$ docker build -t s3cpp-minio:latest .
$ docker run -d -p 9000:9000 -p 9001:9001 \
  -e "MINIO_ROOT_USER=minio_access" \
  -e "MINIO_ROOT_PASSWORD=minio_secret" \
  s3cpp-minio:latest \
  server /data --console-address ":9001"
```

The full test suite contains 63 tests
