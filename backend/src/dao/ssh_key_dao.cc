#include "dao/ssh_key_dao.h"

#include <iostream>

namespace codelab::dao
{
  std::optional<int> SSHKeyDAO::Create(int user_id, const std::string& title, const std::string& key_content) {
    // TODO: insert fingerprint value
    std::string sql = "INSERT INTO ssh_keys (user_id, title, key_content) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db_.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, key_content.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      std::cerr << "[!] Insert SSH Key failed: " << sqlite3_errmsg(db_.GetHandle()) << std::endl;
      sqlite3_finalize(stmt);
      return std::nullopt;
    }

    int id = sqlite3_last_insert_rowid(db_.GetHandle());
    sqlite3_finalize(stmt);
    return id;
  }

  std::vector<SSHKey> SSHKeyDAO::ListByUser(int user_id) {
    std::vector<SSHKey> keys;
    std::string sql = "SELECT id, user_id, title, key_content, fingerprint, created_at FROM ssh_keys WHERE user_id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db_.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return keys;
    sqlite3_bind_int(stmt, 1, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      SSHKey key;
      key.id = sqlite3_column_int(stmt, 0);
      key.user_id = sqlite3_column_int(stmt, 1);
      key.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      key.key_content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

      auto fp = sqlite3_column_text(stmt, 4);
      key.fingerprint = fp ? reinterpret_cast<const char*>(fp) : "";

      key.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
      keys.push_back(key);
    }
    sqlite3_finalize(stmt);
    return keys;
  }

  bool SSHKeyDAO::Delete(int id, int user_id) {
    std::string sql = "DELETE FROM ssh_keys WHERE id = ? AND user_id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db_.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, user_id);

    sqlite3_step(stmt);
    int changes = sqlite3_changes(db_.GetHandle());
    sqlite3_finalize(stmt);
    return changes > 0;
  }

  std::vector<SSHKey> SSHKeyDAO::GetAllKeys() {
    std::vector<SSHKey> keys;
    std::string sql = "SELECT id, user_id, title, key_content, fingerprint, created_at FROM ssh_keys;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db_.GetHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return keys;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      SSHKey key;
      key.id = sqlite3_column_int(stmt, 0);
      key.user_id = sqlite3_column_int(stmt, 1);
      key.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      key.key_content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

      auto fp = sqlite3_column_text(stmt, 4);
      key.fingerprint = fp ? reinterpret_cast<const char*>(fp) : "";

      key.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
      keys.push_back(key);
    }
    sqlite3_finalize(stmt);
    return keys;
  }
}
