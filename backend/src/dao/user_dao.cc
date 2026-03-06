#include "dao/user_dao.h"
#include "core/db.h"
#include <iostream>
#include <sodium.h>

namespace codelab::dao
{
  std::optional<models::User> UserDAO::Create(const std::string &username, const std::string &password, const std::string &email)
  {
    auto& db = core::Database::GetInstance();
    sqlite3* handle = db.GetHandle();
    sqlite3_stmt* stmt;

    char hashed_pwd[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hashed_pwd,
      password.c_str(),
      password.length(),
      crypto_pwhash_OPSLIMIT_INTERACTIVE,
      crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
      std::cerr << "[!] Out of memory for password hashing" << std::endl;
      return std::nullopt;
    }

    std::string sql = "INSERT INTO users (username, password_hash, email) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      std::cerr << "[!] DAO error | Prepare failed: " << sqlite3_errmsg(handle) << std::endl;
      return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_pwd, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, email.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
      std::cerr << "[!] DAO error | Execution failed: " << sqlite3_errmsg(handle) << std::endl;
      sqlite3_finalize(stmt);
      return std::nullopt;
    }

    models::User user;
    user.id = static_cast<int>(sqlite3_last_insert_rowid(db.GetHandle()));
    user.username = username;
    user.password_hash = hashed_pwd;
    user.email = email;

    sqlite3_finalize(stmt);
    return user;
  }

  std::optional<models::User> UserDAO::FindByUsername(const std::string &username)
  {
    auto& db = core::Database::GetInstance();
    sqlite3* handle = db.GetHandle();
    sqlite3_stmt* stmt;

    std::string sql = "SELECT id, username, password_hash, email FROM users WHERE username = ?;";

    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
      models::User user;
      user.id = sqlite3_column_int(stmt, 0);
      user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

      sqlite3_finalize(stmt);
      return user;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
  }

  std::optional<models::User> UserDAO::FindById(int id) {
    auto& db = core::Database::GetInstance();
    sqlite3* handle = db.GetHandle();
    sqlite3_stmt* stmt;

    std::string sql = "SELECT id, username, password_hash, email FROM users WHERE id = ?;";

    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      models::User user;
      user.id = sqlite3_column_int(stmt, 0);
      user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
      sqlite3_finalize(stmt);
      return user;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
  }
}
