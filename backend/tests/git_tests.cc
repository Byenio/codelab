#include <gtest/gtest.h>
#include "git/git_storage.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class GitStorageTest : public ::testing::Test {
protected:
  std::string test_root = "test_repos";

  void SetUp() override {
    fs::create_directory(test_root);
  }

  void TearDown() override {
    fs::remove_all(test_root);
  }
};

TEST_F(GitStorageTest, InitAndExists) {
  codelab::git::GitStorage storage(test_root);
  std::string repo_hash = "aabbcc";

  EXPECT_FALSE(storage.Exists(repo_hash));

  bool created = storage.InitRepo(repo_hash);
  ASSERT_TRUE(created);

  EXPECT_TRUE(storage.Exists(repo_hash));

  std::string head_path = test_root + "/" + repo_hash + ".git/HEAD";
  EXPECT_TRUE(fs::exists(head_path));
}

TEST_F(GitStorageTest, DeleteRepo) {
  codelab::git::GitStorage storage(test_root);
  std::string repo_hash = "ddeeff";

  storage.InitRepo(repo_hash);
  ASSERT_TRUE(storage.Exists(repo_hash));

  bool deleted = storage.DeleteRepo(repo_hash);
  ASSERT_TRUE(deleted);
  EXPECT_FALSE(storage.Exists(repo_hash));
}