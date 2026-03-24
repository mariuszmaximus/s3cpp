#ifndef S3CPP_BENCHMARK_TASKS_H
#define S3CPP_BENCHMARK_TASKS_H

namespace bench {

using ClientHandle = void *;
using initClient_f = ClientHandle(const char *access, const char *secret, const char *endpoint) noexcept;
using get_object_f = const char *(ClientHandle client, const char *key) noexcept;

extern "C" {
__attribute__((visibility("default"))) initClient_f initClient;
__attribute__((visibility("default"))) get_object_f get_object;
}

} // namespace bench

#endif
