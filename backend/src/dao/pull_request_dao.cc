#include "dao/pull_request_dao.h"
#include "core/db.h"
#include <iostream>

namespace codelab::dao
{
  std::optional<int> PullRequestDAO::Create(const models::PullRequest& pr)
  {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "INSERT INTO pull_requests (repository_id, author_id, title, description, source_branch, target_branch, status) VALUES (?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, pr.repository_id);
    sqlite3_bind_int(stmt, 2, pr.author_id);
    sqlite3_bind_text(stmt, 3, pr.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, pr.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, pr.source_branch.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, pr.target_branch.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, pr.status.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      std::cerr << "[!] PR Create Failed: " << sqlite3_errmsg(db.GetHandle()) << std::endl;
      sqlite3_finalize(stmt);
      return std::nullopt;
    }

    int id = (int)sqlite3_last_insert_rowid(db.GetHandle());
    sqlite3_finalize(stmt);
    return id;
  }

  std::optional<models::PullRequest> PullRequestDAO::FindById(int id)
  {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, repository_id, author_id, title, description, source_branch, target_branch, status, created_at FROM pull_requests WHERE id = ?;";

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      models::PullRequest p;
      p.id = sqlite3_column_int(stmt, 0);
      p.repository_id = sqlite3_column_int(stmt, 1);
      p.author_id = sqlite3_column_int(stmt, 2);
      p.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
      auto desc = sqlite3_column_text(stmt, 4);
      p.description = desc ? reinterpret_cast<const char*>(desc) : "";
      p.source_branch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
      p.target_branch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
      p.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
      p.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

      sqlite3_finalize(stmt);
      return p;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
  }

  std::vector<models::PullRequest> PullRequestDAO::ListByRepository(int repository_id)
  {
    std::vector<models::PullRequest> results;
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "SELECT id, repository_id, author_id, title, description, source_branch, target_branch, status, created_at FROM pull_requests WHERE repository_id = ? ORDER BY created_at DESC;";

    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    sqlite3_bind_int(stmt, 1, repository_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      models::PullRequest p;
      p.id = sqlite3_column_int(stmt, 0);
      p.repository_id = sqlite3_column_int(stmt, 1);
      p.author_id = sqlite3_column_int(stmt, 2);
      p.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
      auto desc = sqlite3_column_text(stmt, 4);
      p.description = desc ? reinterpret_cast<const char*>(desc) : "";
      p.source_branch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
      p.target_branch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
      p.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
      p.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
      results.push_back(p);
    }

    sqlite3_finalize(stmt);
    return results;
  }

  bool PullRequestDAO::UpdateStatus(int id, const std::string& status)
  {
    auto& db = core::Database::GetInstance();
    sqlite3_stmt* stmt;

    std::string sql = "UPDATE pull_requests SET status = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(db.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
  }
}
