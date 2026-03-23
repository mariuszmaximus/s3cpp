#include "implementations.h"
#include "tasks.h"

namespace s3b {

void test_get_object(const char *name, get_object_f *get_object) {
  for (const auto key : {"one", "two", "three"}) {
    std::println("{}", get_object(key));
  }
}

} // namespace s3b

int main() {
  std::array<s3b::implementation, s3b::implementations_count> implementations =
      s3b::parse_implementations();
  for (const auto &implementation : implementations) {
    s3b::test_get_object(implementation.name, implementation.get_object);
  }
}
