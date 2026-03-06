#include <crow.h>
#include <git2.h>
#include <iostream>
#include <sodium.h>

#include "api/routes.h"
#include "core/config.h"
#include "core/db.h"
#include "middleware/auth_middleware.h"

int main(int argc, char** argv) {
  auto& config = codelab::core::Config::GetInstance();
  config.Load("../../.env");

  if (sodium_init() < 0)
  {
    std::cerr << "[!] Failed to initialize libsodium" << std::endl;
    return 1;
  }

  git_libgit2_init();

  try
  {
    auto& db = codelab::core::Database::GetInstance();
    std::string db_path = config.GetString("DB_PATH", "../../data/codelab.db");
    db.Connect(db_path);
    db.ApplySchema("db/schema.sql");
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
