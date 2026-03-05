#include <crow.h>
#include <git2.h>
#include <iostream>
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

  CROW_ROUTE(app, "/api/v1/register").methods(crow::HTTPMethod::POST) ([](const crow::request& req)
  {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");

    std::string username = x["username"].s();
    std::string password = x["password"].s();
    std::string email = x["email"].s();

    codelab::dao::UserDAO dao;

    // TODO: check if user exists using dao.FindByXyz(username)

    if (auto user = dao.Create(username, password, email))
    {
      crow::json::wvalue res;
      res["id"] = user->id;
      res["username"] = user->username;
      return crow::response(201, res);
    } else
    {
      return crow::response(500, "Failed to create user");
    }
  });

  CROW_ROUTE(app, "/api/test_structure")([](){
    codelab::dao::UserDAO userDao;
    codelab::dao::DirectoryDAO dirDao;
    codelab::dao::RepositoryDAO repoDao;

    int user_id = 0;
    auto existing_user = userDao.FindByUsername("tester");

    if (existing_user) {
        user_id = existing_user->id;
    } else {
        auto new_user = userDao.Create("tester", "hashed_secret", "test@code.lab");
        if (!new_user) return crow::response(500, "[!] Failed to create test user");
        user_id = new_user->id;
    }

    auto root_id = dirDao.Create(user_id, std::nullopt, "polsl");
    if (!root_id) return crow::response(500, "[!] Failed to create root directory");

    auto embedded_id = dirDao.Create(user_id, *root_id, "sem-6");
    if (!embedded_id) return crow::response(500, "Failed to create embedded directory");

    codelab::services::RepoService repo_service;

    codelab::models::Repository repo = repo_service.CreateRepository(user_id, embedded_id, "project", "test project", true).value();

    if (repo.id) {
        return crow::response(200, "created structure: User(" + std::to_string(user_id) + ")/polsl/sem-6/" + repo.name);
    }
    return crow::response(500, "[!] Failed to create repo");
  });

  std::cout << "[+] Starting codelab server on port 8080..." << std::endl;
  app.port(8080).multithreaded().run();

  git_libgit2_shutdown();
  return 0;
}
