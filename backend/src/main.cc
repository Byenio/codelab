#include <crow.h>
#include <sqlite3.h>
#include <git2.h>
#include <iostream>

int main() {
  // 1. Initialize Libgit2
  git_libgit2_init();
  std::cout << "[+] Libgit2 initialized" << std::endl;

  // 2. Initialize SQLite
  sqlite3* db;
  int rc = sqlite3_open(":memory:", &db); // In-memory DB for test
  if (rc) {
    std::cerr << "[-] Can't open database: " << sqlite3_errmsg(db) << std::endl;
    return 1;
  } else {
    std::cout << "[+] SQLite3 initialized (in-memory)" << std::endl;
  }
  sqlite3_close(db);

  // 3. Start Crow Server
  crow::SimpleApp app;

  CROW_ROUTE(app, "/")([](){
      return "codelab backend is running!";
  });

  CROW_ROUTE(app, "/status")([](){
      crow::json::wvalue x;
      x["service"] = "codelab";
      x["status"] = "online";
      x["git_version"] = "libgit2";
      return x;
  });

  std::cout << "[+] Starting codelab server on port 8080..." << std::endl;

  // Shutdown libgit2 on exit
  // In a real app, use RAII or atexit
  app.port(8080).multithreaded().run();

  git_libgit2_shutdown();
  return 0;
}