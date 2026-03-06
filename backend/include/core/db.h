#ifndef CODELAB_DB_H
#define CODELAB_DB_H

#include <sqlite3.h>
#include <string>
#include <stdexcept>

namespace codelab::core
{
  /**
   * @brief Manages SQLite connection.
   *
   * Usage:
   *   auto& db = codelab::core::Database::GetInstance();
   *   sqlite3* raw_handle = db.GetHandle();
   */
  class Database
  {
  public:
    static Database& GetInstance();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * @brief Opens database connection.
     * @param db_path Path to sqlite file
     */
    void Connect(const std::string& db_path);

    /**
     * @brief Reads schema.sql and applies it to the database.
     * @param schema_path Path to schema.sql
     */
    void ApplySchema(const std::string& schema_path);

    /**
     * @brief Returns raw sqlite3 handle for use in repositories.
     */
    sqlite3* GetHandle() const;

    void Disconnect();

  private:
    Database() = default;
    ~Database();

    sqlite3* db_ = nullptr;
  };
}

#endif //CODELAB_DB_H