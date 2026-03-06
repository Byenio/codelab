#include "test_database.h"
#include "services/auth_service.h"
#include "dao/user_dao.h"

class AuthTest : public DBTest {};

TEST_F(AuthTest, ValidLoginReturnsToken) {
  codelab::dao::UserDAO dao;
  dao.Create("alice", "mypassword", "alice@test.com");

  codelab::services::AuthService auth;
  auto token = auth.Login("alice", "mypassword");

  ASSERT_TRUE(token.has_value());
  EXPECT_FALSE(token->empty());
}

TEST_F(AuthTest, InvalidLoginFails) {
  codelab::dao::UserDAO dao;
  dao.Create("bob", "correctpass", "bob@test.com");

  codelab::services::AuthService auth;

  // Wrong password
  auto token1 = auth.Login("bob", "wrongpass");
  EXPECT_FALSE(token1.has_value());

  // Wrong user
  auto token2 = auth.Login("charlie", "correctpass");
  EXPECT_FALSE(token2.has_value());
}

TEST_F(AuthTest, VerifyValidToken) {
  codelab::dao::UserDAO dao;
  auto user = dao.Create("dave", "pass", "dave@test.com");

  codelab::services::AuthService auth;
  auto token = auth.Login("dave", "pass");
  ASSERT_TRUE(token.has_value());

  auto user_id = auth.VerifyToken(*token);
  ASSERT_TRUE(user_id.has_value());
  EXPECT_EQ(*user_id, user->id);
}

TEST_F(AuthTest, VerifyInvalidTokenFails) {
  codelab::services::AuthService auth;
  auto user_id = auth.VerifyToken("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.invalid.signature");
  EXPECT_FALSE(user_id.has_value());
}