#include <gtest/gtest.h>
#include <git2.h>
#include <sodium/core.h>

int main(int argc, char **argv) {
  if (sodium_init() < 0) {
    std::cerr << "[!] Fatal: sodium_init() failed!" << std::endl;
    return 1;
  }

  ::testing::InitGoogleTest(&argc, argv);

  git_libgit2_init();

  int result = RUN_ALL_TESTS();

  git_libgit2_shutdown();
  return result;
}
