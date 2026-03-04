#include "dao/user_dao.h"
#include "core/db.h"
#include <iostream>

namespace codelab::dao
{
  std::optional<models::User> UserDAO::Create(const std::string &username, const std::string &password, const std::string &email)
  {
    auto& db = core::Database::GetInstance();
    sqlite3* handle = db.GetHandle();
    sqlite3_stmt* stmt;

    std::string sql = "INSERT INTO users (username, password_hash, email) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      std::cerr << "[!] DAO error | Prepare failed: " << sqlite3_errmsg(handle) << std::endl;
      return std::nullopt;
    }

    // TODO: Hash password before storing
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, email.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
      std::cerr << "[!] DAO error | Execution failed: " << sqlite3_errmsg(handle) << std::endl;
      sqlite3_finalize(stmt);
      return std::nullopt;
    }

    int id = (int)sqlite3_last_insert_rowid(handle);
    sqlite3_finalize(stmt);

    return models::User(id, username, password, email);
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
