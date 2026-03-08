#include <gtest/gtest.h>
#include "git/git_storage.h"
#include "git/git_viewer.h"
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
  std::string name = "repository1";

  EXPECT_FALSE(storage.Exists(repo_hash));

  bool created = storage.InitRepo(repo_hash, name, false); // false = no readme
  ASSERT_TRUE(created);

  EXPECT_TRUE(storage.Exists(repo_hash));

  std::string head_path = test_root + "/" + repo_hash + ".git/HEAD";
  EXPECT_TRUE(fs::exists(head_path));
}

TEST_F(GitStorageTest, DeleteRepo) {
  codelab::git::GitStorage storage(test_root);
  std::string repo_hash = "ddeeff";
  std::string name = "repository2";

  storage.InitRepo(repo_hash, name, false);
  ASSERT_TRUE(storage.Exists(repo_hash));

  bool deleted = storage.DeleteRepo(repo_hash);
  ASSERT_TRUE(deleted);
  EXPECT_FALSE(storage.Exists(repo_hash));
}

TEST_F(GitStorageTest, InitWithReadmeAndViewContents) {
  codelab::git::GitStorage storage(test_root);
  std::string repo_hash = "readme_test";

  bool created = storage.InitRepo(repo_hash, "ReadmeProject", true);
  ASSERT_TRUE(created);

  std::string full_path = test_root + "/" + repo_hash + ".git";
  codelab::git::GitViewer viewer(full_path);

  auto branches = viewer.GetBranches();
  ASSERT_EQ(branches.size(), 1);
  EXPECT_EQ(branches[0].name, "master");

  auto commits = viewer.GetCommits("master");
  ASSERT_EQ(commits.size(), 1);
  EXPECT_EQ(commits[0].message, "initial commit");
  EXPECT_EQ(commits[0].author_name, "Codelab System");

  auto tree = viewer.GetTree("HEAD");
  ASSERT_EQ(tree.size(), 1);
  EXPECT_EQ(tree[0].name, "README.md");
  EXPECT_EQ(tree[0].type, "blob");

  auto blob = viewer.GetBlob("HEAD", "README.md");
  ASSERT_TRUE(blob.has_value());
  EXPECT_TRUE(blob->find("# ReadmeProject") != std::string::npos);
}