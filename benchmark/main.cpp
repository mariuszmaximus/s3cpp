#include "implementations.h"
#include "tasks.h"
#include <print>

using namespace s3b;

int main(int argc, char *argv[]) {
  std::array<implementation, implementations_count> implementations =
      parse_implementations();
  return 0;
}
