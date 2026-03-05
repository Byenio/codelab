#include "api/routes.h"
#include "dao/repository_dao.h"
#include "dao/directory_dao.h"
#include "services/repo_service.h"

namespace codelab::api
{
  void RegisterRoutes(crow::App<middleware::AuthMiddleware>& app)
  {
    // --- DEBUG ROUTES ---

    CROW_ROUTE(app, "/api/v1/health") ([]()
    {
      crow::json::wvalue res;
      res["status"] = "ok";
      res["db"] = "connected";
      return res;
    });

    // --- DIRECTORY ROUTES ---

    // List contents of a folder
    // GET /api/directories?parent_id=0
    CROW_ROUTE(app, "/api/v1/directories")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req)
    {
      // TODO: get user_id from middleware context, once auth is built
      int user_id = 1;

      std::optional<int> parent_id = std::nullopt;
      if (req.url_params.get("parent_id"))
      {
        parent_id = std::stoi(req.url_params.get("parent_id"));
      }

      dao::DirectoryDAO dir_dao;
      dao::RepositoryDAO repo_dao;

      auto dirs = dir_dao.ListByParent(user_id, parent_id);

      auto repos = repo_dao.ListByDirectory(user_id, parent_id);

      crow::json::wvalue res;
      res["directories"] = crow::json::wvalue::list();
      res["repositories"] = crow::json::wvalue::list();

      int i = 0;
      for (const auto& d : dirs)
      {
        res["directories"][i]["id"] = d.id;
        res["directories"][i]["name"] = d.name;
        res["directories"][i]["type"] = "directory";
        i++;
      }

      int j = 0;
      for (const auto& r : repos)
      {
        res["repositories"][j]["id"] = r.id;
        res["repositories"][j]["name"] = r.name;
        res["repositories"][j]["type"] = "repository";
        res["repositories"][j]["is_private"] = r.is_private;
        j++;
      }

      return res;
    });

    // Create new folder
    // POST /api/directories
    CROW_ROUTE(app, "/api/v1/directories")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req)
    {
      // TODO: get user_id from middleware context, once auth is built
      int user_id = 1;
      auto data = crow::json::load(req.body);

      if (!data) return crow::response(400, "Invalid JSON");

      std::string name= data["name"].s();
      std::optional<int> parent_id;

      if (data.has("parent_id") && data["parent_id"].t() == crow::json::type::Number)
      {
        parent_id = static_cast<int>(data["parent_id"].i());
      }

      dao::DirectoryDAO dir_dao;
      auto id = dir_dao.Create(user_id, parent_id, name);

      if (id) return crow::response(201, "Directory created");
      return crow::response(500, "Failed to create directory");
    });

    // --- REPOSITORY ROUTES ---

    // Create new repo
    // POST /api/repositories
    CROW_ROUTE(app, "/api/v1/repositories")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req)
    {
      // TODO: get user_id from middleware context, once auth is built
      int user_id = 1;
      auto data = crow::json::load(req.body);

      if (!data) return crow::response(400, "Invalid JSON");

      std::string name= data["name"].s();
      std::string description= data.has("description") ? data["description"].s() : static_cast<std::string>("");
      bool is_private = data["is_private"].b();
      std::optional<int> dir_id;

      if (data.has("directory_id") && data["directory_id"].t() == crow::json::type::Number)
      {
        dir_id = static_cast<int>(data["directory_id"].i());
      }

      services::RepoService repo_service;
      auto result = repo_service.CreateRepository(user_id, dir_id, name, description, is_private);

      if (result) return crow::response(201, "Repository created");
      return crow::response(500, "Failed to create repository");
    });
  }
}