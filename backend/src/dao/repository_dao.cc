#include "dao/repository_dao.h"
#include "core/db.h"
#include <iostream>

namespace codelab::dao
{
  std::optional<int> RepositoryDAO::Create(const models::Repository &repo)
  {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "INSERT INTO repositories (directory_id, user_id, name, description, is_private, disk_path_hash) VALUES (?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    if (repo.directory_id.has_value()) sqlite3_bind_int(stmt, 1, repo.directory_id.value());
    else sqlite3_bind_null(stmt, 1);

    sqlite3_bind_int(stmt, 2, repo.user_id);
    sqlite3_bind_text(stmt, 3, repo.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, repo.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, repo.is_private ? 1 : 0);
    sqlite3_bind_text(stmt, 6, repo.disk_path_hash.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      std::cerr << "[!] Repo Create Failed: " << sqlite3_errmsg(db.GetHandle()) << std::endl;
      sqlite3_finalize(stmt);
      return std::nullopt;
    }

    int id = (int)sqlite3_last_insert_rowid(db.GetHandle());
    sqlite3_finalize(stmt);
    return id;
  }

  std::optional<models::Repository> RepositoryDAO::FindByName(int user_id, std::optional<int> directory_id, const std::string& name) {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql;
    if (directory_id.has_value())
    {
      sql = "SELECT id, user_id, directory_id, name, description, disk_path_hash, is_private, created_at "
          "FROM repositories WHERE name=? AND ( (user_id=? AND directory_id=?) "
          "OR id IN (SELECT repository_id FROM collaborators WHERE user_id=?) );";
    } else
    {
      sql = "SELECT id, user_id, directory_id, name, description, disk_path_hash, is_private, created_at "
          "FROM repositories WHERE name=? AND ( (user_id=? AND directory_id IS NULL) "
          "OR id IN (SELECT repository_id FROM collaborators WHERE user_id=?) );";
    }

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);

    if (directory_id.has_value()) {
      sqlite3_bind_int(stmt, 3, directory_id.value());
      sqlite3_bind_int(stmt, 4, user_id);
    } else {
      sqlite3_bind_int(stmt, 3, user_id);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      models::Repository r;
      r.id = sqlite3_column_int(stmt, 0);
      r.user_id = sqlite3_column_int(stmt, 1);
      if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) r.directory_id = sqlite3_column_int(stmt, 2);
      r.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
      auto desc = sqlite3_column_text(stmt, 4);
      r.description = desc ? reinterpret_cast<const char*>(desc) : "";
      r.disk_path_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
      r.is_private = sqlite3_column_int(stmt, 6) != 0;
      r.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
      sqlite3_finalize(stmt);
      return r;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
  }

  std::optional<models::Repository> RepositoryDAO::FindById(int id) {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, directory_id, name, description, disk_path_hash, is_private, created_at FROM repositories WHERE id = ?;";
    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      models::Repository r;
      r.id = sqlite3_column_int(stmt, 0);
      r.user_id = sqlite3_column_int(stmt, 1);
      if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) r.directory_id = sqlite3_column_int(stmt, 2);
      r.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

      auto desc = sqlite3_column_text(stmt, 4);
      r.description = desc ? reinterpret_cast<const char*>(desc) : "";

      r.disk_path_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
      r.is_private = sqlite3_column_int(stmt, 6) != 0;
      r.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

      sqlite3_finalize(stmt);
      return r;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
  }

  std::vector<models::Repository> RepositoryDAO::ListByDirectory(int user_id, std::optional<int> directory_id)
  {
    std::vector<models::Repository> results;
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql;
    if (directory_id.has_value())
    {
      sql = "SELECT id, user_id, directory_id, name, description, disk_path_hash, is_private, created_at FROM repositories WHERE user_id=? AND directory_id=?;";
    } else
    {
      sql = "SELECT id, user_id, directory_id, name, description, disk_path_hash, is_private, created_at FROM repositories WHERE user_id=? AND directory_id IS NULL;";
    }

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    sqlite3_bind_int(stmt, 1, user_id);
    if (directory_id.has_value())
    {
      sqlite3_bind_int(stmt, 2, directory_id.value());
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      models::Repository repo;
      repo.id = sqlite3_column_int(stmt, 0);
      repo.user_id = sqlite3_column_int(stmt, 1);
      if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      {
        repo.directory_id = sqlite3_column_int(stmt, 2);
      }
      repo.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
      auto desc = sqlite3_column_text(stmt, 4);
      repo.description = desc ? reinterpret_cast<const char*>(desc) : "";
      repo.disk_path_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
      repo.is_private = sqlite3_column_int(stmt, 6) != 0;
      repo.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
      results.push_back(repo);
    }

    sqlite3_finalize(stmt);
    return results;
  }

  std::vector<models::Repository> RepositoryDAO::ListByUser(int user_id)
  {
    std::vector<models::Repository> results;
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "SELECT id, user_id, directory_id, name, description, disk_path_hash, is_private, created_at FROM repositories WHERE user_id=? ORDER BY created_at DESC;";

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    sqlite3_bind_int(stmt, 1, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      models::Repository repo;
      repo.id = sqlite3_column_int(stmt, 0);
      repo.user_id = sqlite3_column_int(stmt, 1);
      if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      {
        repo.directory_id = sqlite3_column_int(stmt, 2);
      }
      repo.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
      auto desc = sqlite3_column_text(stmt, 4);
      repo.description = desc ? reinterpret_cast<const char*>(desc) : "";
      repo.disk_path_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
      repo.is_private = sqlite3_column_int(stmt, 6) != 0;
      repo.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
      results.push_back(repo);
    }

    sqlite3_finalize(stmt);
    return results;
  }

  bool RepositoryDAO::Delete(int id)
  {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "DELETE FROM repositories WHERE id=?;";
    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
  }

  bool RepositoryDAO::UpdateDirectory(int id, std::optional<int> new_directory_id)
  {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "UPDATE repositories SET directory_id=? WHERE id=?;";
    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    if (new_directory_id.has_value()) {
      sqlite3_bind_int(stmt, 1, new_directory_id.value());
    } else {
      sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
  }

  bool RepositoryDAO::AddCollaborator(int repo_id, int user_id)
  {
    std::string q = "INSERT OR IGNORE INTO collaborators (repository_id, user_id) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(core::Database::GetInstance().GetHandle(), q.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      return false;
    }

    sqlite3_bind_int(stmt, 1, repo_id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
  }

  bool RepositoryDAO::RemoveCollaborator(int repo_id, int user_id)
  {
    std::string q = "DELETE FROM collaborators WHERE repository_id = ? AND user_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(core::Database::GetInstance().GetHandle(), q.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      return false;
    }

    sqlite3_bind_int(stmt, 1, repo_id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
  }

  bool RepositoryDAO::IsCollaborator(int repo_id, int user_id)
  {
    std::string q = "SELECT 1 FROM collaborators WHERE repository_id = ? AND user_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(core::Database::GetInstance().GetHandle(), q.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      return false;
    }

    sqlite3_bind_int(stmt, 1, repo_id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool is_collab = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return is_collab;
  }

  std::vector<int> RepositoryDAO::ListCollaboratorIds(int repo_id)
  {
    std::vector<int> ids;
    std::string q = "SELECT user_id FROM collaborators WHERE repository_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(core::Database::GetInstance().GetHandle(), q.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
      return ids;
    }

    sqlite3_bind_int(stmt, 1, repo_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      ids.push_back(sqlite3_column_int(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return ids;
  }
}
