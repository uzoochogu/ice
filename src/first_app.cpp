#include "ice.hpp"
#include <stdexcept>

int main() {
  ice::Ice block;

  try {
    block.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}