#include "implementations.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <print>
#include <string>

namespace s3b {

static void benchmark_init_client(::benchmark::State &state, s3b::implementation impl) {
  const std::string access_ = "minio_access";
  const std::string secret_ = "minio_secret";
  const std::string endpoint_ = "127.0.0.1:9000";
  for (auto _ : state) {
    ::benchmark::DoNotOptimize(impl.init_client("minio_access", "minio_secret", "127.0.0.1:9000"));
  }
}

static void benchmark_create_bucket(::benchmark::State &state, s3b::implementation impl) {
  // TODO(cristian): Client handler should be stored on the implementation
  // struct so that we only have one client at a time initialized, shouldn't
  // affect the benches but who knows
  bench::ClientHandle handle = impl.init_client("minio_access", "minio_secret", "127.0.0.1:9000");

  // Ugly way of avoiding BucketAlreadyOwnedByYou error 
  static std::atomic<int> run_id{0};
  int run = run_id++;
  int i = 0;

  for (auto _ : state) {
    state.PauseTiming();
    auto bucket = std::format("bench-{}-{}-{}", s3b::bucket_name_for(impl.name), run, i++);
    state.ResumeTiming();
    impl.create_bucket(handle, bucket.c_str());
  }
}

static std::string get_benchmark_name(const std::string &aws_operation, const implementation &impl) {
  const std::string so_name = std::filesystem::path(impl.name).stem();
  const std::string bench_name = std::format("BENCHMARK_{}_{}", so_name, aws_operation);
  return bench_name;
}

} // namespace s3b

int main(int argc, char *argv[]) {
  s3b::check_endpoint("127.0.0.1:9000");
  s3b::clean_up();

  std::array<s3b::implementation, s3b::implementations_count> implementations = s3b::parse_implementations();

  for (const auto &impl : implementations) {
    ::benchmark::RegisterBenchmark(get_benchmark_name("initclient", impl), s3b::benchmark_init_client, impl);
    ::benchmark::RegisterBenchmark(get_benchmark_name("createbucket", impl), s3b::benchmark_create_bucket,
                                   impl);
  }

  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();

  s3b::clean_up();

  ::benchmark::Shutdown();
}
