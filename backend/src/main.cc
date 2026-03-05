#include <crow.h>
#include <git2.h>
#include <iostream>

#include "api/routes.h"
#include "core/db.h"
#include "dao/user_dao.h"
#include "dao/repository_dao.h"
#include "dao/directory_dao.h"
#include "middleware/auth_middleware.h"
#include "services/repo_service.h"

int main(int argc, char** argv) {
  git_libgit2_init();

  try
  {
    auto& db = codelab::core::Database::GetInstance();
    db.Connect("../../data/codelab.db");
    db.ApplySchema("db/schema.sql");
  } catch (const std::exception& e)
  {
    std::cerr << "[!] Database error: " << e.what() << std::endl;
    git_libgit2_shutdown();
    return 1;
  }

  crow::App<codelab::middleware::AuthMiddleware> app;
  codelab::api::RegisterRoutes(app);

  std::cout << "[+] Starting codelab server on port 8080..." << std::endl;
  app.port(8080).multithreaded().run();

  git_libgit2_shutdown();
  return 0;
}
