#include <gtest/gtest.h>
#include "test_database.h"
#include "dao/user_dao.h"
#include "dao/directory_dao.h"

// Inherit from DBTest to get fresh DB every time
class DaoTest : public DBTest {};

TEST_F(DaoTest, CreateUser) {
  codelab::dao::UserDAO dao;
  auto user = dao.Create("testuser", "secret", "email@test.com");

  ASSERT_TRUE(user.has_value());
  EXPECT_EQ(user->username, "testuser");
  EXPECT_EQ(user->id, 1);
}

TEST_F(DaoTest, CreateDirectory) {
  // 1. Setup User
  codelab::dao::UserDAO userDao;
  auto user = userDao.Create("u", "p", "e");
  ASSERT_TRUE(user.has_value());

  // 2. Test Directory
  codelab::dao::DirectoryDAO dirDao;
  auto dirId = dirDao.Create(user->id, std::nullopt, "RootFolder");

  ASSERT_TRUE(dirId.has_value());

  // 3. Verify
  auto dirs = dirDao.ListByParent(user->id, std::nullopt);
  ASSERT_EQ(dirs.size(), 1);
  EXPECT_EQ(dirs[0].name, "RootFolder");
}

TEST_F(DaoTest, NestedDirectories) {
  codelab::dao::UserDAO uDao;
  codelab::dao::DirectoryDAO dDao;
  int uid = uDao.Create("u", "p", "e")->id;

  auto parent = dDao.Create(uid, std::nullopt, "Parent");
  auto child = dDao.Create(uid, *parent, "Child");

  auto rootDirs = dDao.ListByParent(uid, std::nullopt);
  EXPECT_EQ(rootDirs.size(), 1);

  auto subDirs = dDao.ListByParent(uid, *parent);
  EXPECT_EQ(subDirs.size(), 1);
  EXPECT_EQ(subDirs[0].name, "Child");
}