#include "test_database.h"
#include "services/repo_service.h"
#include "dao/user_dao.h"
#include "dao/directory_dao.h"
#include "git/git_viewer.h"
#include <filesystem>

namespace fs = std::filesystem;

class ServiceTest : public DBTest {
protected:
  std::string test_repo_root = "test_service_repos";

  void SetUp() override {
    DBTest::SetUp();
    fs::create_directory(test_repo_root);
  }

  void TearDown() override {
    DBTest::TearDown();
    fs::remove_all(test_repo_root);
  }
};

TEST_F(ServiceTest, CreateRepoSuccess) {
  codelab::dao::UserDAO uDao;
  codelab::dao::DirectoryDAO dDao;
  int uid = uDao.Create("u", "p", "e")->id;
  int dirId = *dDao.Create(uid, std::nullopt, "ProjectFolder");

  codelab::services::RepoService service(test_repo_root);

  auto repo = service.CreateRepository(uid, dirId, "MyRepo", "Desc", true, false);

  ASSERT_TRUE(repo.has_value());
  EXPECT_EQ(repo->name, "MyRepo");
  EXPECT_FALSE(repo->disk_path_hash.empty());

  std::string expected_path = test_repo_root + "/" + repo->disk_path_hash + ".git";
  EXPECT_TRUE(fs::exists(expected_path));
}

TEST_F(ServiceTest, DuplicateNameFails) {
  codelab::dao::UserDAO uDao;
  int uid = uDao.Create("u", "p", "e")->id;
  codelab::services::RepoService service(test_repo_root);

  auto r1 = service.CreateRepository(uid, std::nullopt, "SameName", "", true, false);
  ASSERT_TRUE(r1.has_value());

  auto r2 = service.CreateRepository(uid, std::nullopt, "SameName", "", true, false);
  EXPECT_FALSE(r2.has_value());
}

TEST_F(ServiceTest, CreateRepoWithReadme) {
  codelab::dao::UserDAO uDao;
  int uid = uDao.Create("user2", "pass2", "email2")->id;
  codelab::services::RepoService service(test_repo_root);

  auto repo = service.CreateRepository(uid, std::nullopt, "ReadmeRepo", "Desc", true, true);
  ASSERT_TRUE(repo.has_value());

  std::string expected_path = test_repo_root + "/" + repo->disk_path_hash + ".git";
  codelab::git::GitViewer viewer(expected_path);

  auto tree = viewer.GetTree("HEAD");
  ASSERT_EQ(tree.size(), 1);
  EXPECT_EQ(tree[0].name, "README.md");
}