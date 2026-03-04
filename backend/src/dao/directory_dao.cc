#include "dao/directory_dao.h"
#include "core/db.h"
#include <iostream>

namespace codelab::dao
{
  std::optional<int> DirectoryDAO::Create(int user_id, std::optional<int> parent_id, const std::string &name)
  {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "INSERT INTO directories (user_id, parent_id, name) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, user_id);
    if (parent_id.has_value())
    {
      sqlite3_bind_int(stmt, 2, parent_id.value());
    } else
    {
      sqlite3_bind_null(stmt, 2);
    }
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
      std::cerr << "[!] Creation of directory failed: " << sqlite3_errmsg(db.GetHandle()) << std::endl;
      sqlite3_finalize(stmt);
      return std::nullopt;
    }

    int id = static_cast<int>(sqlite3_last_insert_rowid(db.GetHandle()));
    sqlite3_finalize(stmt);
    return id;
  }

  std::vector<models::Directory> DirectoryDAO::ListByParent(int user_id, std::optional<int> parent_id)
  {
    std::vector<models::Directory> results;
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql;

    if (parent_id.has_value())
    {
      sql = "SELECT id, user_id, parent_id, name FROM directories WHERE user_id = ? AND parent_id = ?;";
    } else
    {
      sql = "SELECT id, user_id, parent_id, name FROM directories WHERE user_id = ? AND parent_id IS NULL;";
    }

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    sqlite3_bind_int(stmt, 1, user_id);
    if (parent_id.has_value())
    {
      sqlite3_bind_int(stmt, 2, parent_id.value());
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      models::Directory dir;
      dir.id = sqlite3_column_int(stmt, 0);
      dir.user_id = sqlite3_column_int(stmt, 1);
      if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      {
        dir.parent_id = sqlite3_column_int(stmt, 2);
      }
      dir.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

      results.push_back(dir);
    }

    sqlite3_finalize(stmt);
    return results;
  }
}
