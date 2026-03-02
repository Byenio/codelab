#include <crow.h>
#include <git2.h>
#include <iostream>
#include "core/db.h"

int main(int argc, char** argv) {
  git_libgit2_init();

  try
  {
    auto& db = codelab::core::Database::GetInstance();

    db.Connect("../../data/codelab.db");

    db.ApplySchema("db/schema.sql");
  } catch (const std::exception& e)
  {
    std::cerr << "[!] Initialization error: " << e.what() << std::endl;
    git_libgit2_shutdown();

    return 1;
  }

  crow::SimpleApp app;

  CROW_ROUTE(app, "/") ([]()
  {
    return "Codelab is running";
  });

  CROW_ROUTE(app, "/health") ([]()
  {
    crow::json::wvalue res;
    res["status"] = "ok";
    res["db"] = "connected";
    return res;
  });

  std::cout << "[+] Starting codelab server on port 8080..." << std::endl;
  app.port(8080).multithreaded().run();

  git_libgit2_shutdown();
  return 0;
}