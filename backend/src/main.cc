#include <crow.h>
#include <git2.h>
#include <iostream>
#include <sodium.h>

#include "api/routes.h"
#include "core/config.h"
#include "core/db.h"
#include "middleware/auth_middleware.h"
#include "services/ssh_service.h"

int main(int argc, char** argv) {
  auto& config = codelab::core::Config::GetInstance();
  config.Load(".env");

  if (sodium_init() < 0)
  {
    std::cerr << "[!] Failed to initialize libsodium" << std::endl;
    return 1;
  }

  if (git_libgit2_init() < 0)
  {
    std::cerr << "[!] Failed to initialize libgit2" << std::endl;
    return 1;
  }

  try
  {
    auto& db = codelab::core::Database::GetInstance();
    std::string db_path = config.GetString("DB_PATH", "../../data/codelab.db");
    db.Connect(db_path);
    std::string db_schema_path = config.GetString("DB_SCHEMA_PATH", "db/schema.sql");
    db.ApplySchema(db_schema_path);

    codelab::services::SSHService ssh_service;
    if (ssh_service.SyncAuthorizedKeysFile())
    {
      std::cout << "[+] SSH key synced successfully" << std::endl;
    } else
    {
      std::cerr << "[!] Failed to sync SSH keys" << std::endl;
    }
  } catch (const std::exception& e)
  {
    std::cerr << "[!] Database error: " << e.what() << std::endl;
    git_libgit2_shutdown();
    return 1;
  }

  crow::App<codelab::middleware::AuthMiddleware> app;
  codelab::api::RegisterRoutes(app);

  int app_port = config.GetInt("PORT", 8080);
  std::cout << "[+] Starting codelab server on port " << app_port << std::endl;
  app.port(app_port).multithreaded().run();

  git_libgit2_shutdown();
  return 0;
}
