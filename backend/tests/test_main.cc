#include <gtest/gtest.h>
#include <git2.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  git_libgit2_init();

  int result = RUN_ALL_TESTS();

  git_libgit2_shutdown();
  return result;
}