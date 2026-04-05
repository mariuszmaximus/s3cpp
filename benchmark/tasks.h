#ifndef S3CPP_BENCHMARK_TASKS_H
#define S3CPP_BENCHMARK_TASKS_H

namespace bench {

using ClientHandle = void *;
using init_client_f = ClientHandle(const char *access, const char *secret, const char *endpoint) noexcept;
using create_bucket_f = void(ClientHandle client, const char *bucketName) noexcept;
using put_object_f = void(ClientHandle client, const char *bucket, const char* key, const char* contents) noexcept;
using get_object_f = const char *(ClientHandle client, const char *bucket, const char *key) noexcept;
using free_client_f = void(ClientHandle client) noexcept;

extern "C" {
__attribute__((visibility("default"))) init_client_f init_client;
__attribute__((visibility("default"))) create_bucket_f create_bucket;
__attribute__((visibility("default"))) put_object_f put_object;
__attribute__((visibility("default"))) get_object_f get_object;
__attribute__((visibility("default"))) free_client_f free_client;
}

} // namespace bench

#endif
