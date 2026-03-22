#ifndef S3CPP_BENCHMARK_TASK_H
#define S3CPP_BENCHMARK_TASK_H

namespace s3bench {

typedef const char* get_object_f(const char* key) noexcept;

extern "C" 
__attribute__((visibility("default")))
get_object_f get_object;

} // namespace s3bench

#endif
