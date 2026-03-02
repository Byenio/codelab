#include <core/db.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace codelab::core
{
  Database &Database::GetInstance()
  {
    static Database instance;
    return instance;
  }

  Database::~Database()
  {
    if (db_)
    {
      sqlite3_close(db_);
      std::cout << "[-] Database connection closed" << std::endl;
    }
  }

  void Database::Connect(const std::string& db_path)
  {
    if (db_) return;

    std::filesystem::path path(db_path);
    if (path.has_parent_path())
    {
      std::filesystem::create_directories(path.parent_path());
    }

    int rc = sqlite3_open(db_path.c_str(), &db_);

    if (rc != SQLITE_OK)
    {
      std::string err = sqlite3_errmsg(db_);
      sqlite3_close(db_);
      db_ = nullptr;
      throw std::runtime_error("Failed to open database: " + err);
    }

    char* err_message = nullptr;
    rc = sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &err_message);
    if (rc != SQLITE_OK)
    {
      std::string err = err_message;
      sqlite3_free(err_message);
      throw std::runtime_error("Failed to enable foreign keys: " + err);
    }

    std::cout << "[+] Database connected at: " << db_path << std::endl;
  }

  void Database::ApplySchema(const std::string &schema_path)
  {
    if (!db_) throw std::runtime_error("Database not connected");

    std::ifstream file(schema_path);
    if (!file.is_open())
    {
      throw std::runtime_error("Could not open schema file: " + schema_path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    char* err_message = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_message);
    if (rc != SQLITE_OK)
    {
      std::string err = err_message;
      sqlite3_free(err_message);
      throw std::runtime_error("Schema execution failed: " + err);
    }

    std::cout << "[+] Schema applied successfully" << std::endl;
  }

  sqlite3 *Database::GetHandle() const
  {
    return db_;
  }
}
