#ifndef S3CPP_BENCHMARK_TASKS_H
#define S3CPP_BENCHMARK_TASKS_H

namespace s3b {

typedef const char* init_f(const char* access, const char* secret, const char* endpoint) noexcept;
typedef const char* get_object_f(const char* key) noexcept;

extern "C" {
__attribute__((visibility("default"))) init_f init;
__attribute__((visibility("default"))) get_object_f get_object;
}

} // namespace s3bench

#endif
