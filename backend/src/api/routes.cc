#include "api/routes.h"

#include "core/config.h"
#include "dao/repository_dao.h"
#include "dao/directory_dao.h"
#include "git/git_viewer.h"
#include "services/repo_service.h"

namespace codelab::api
{
  void RegisterRoutes(crow::App<middleware::AuthMiddleware>& app)
  {
    // region --- PUBLIC ROUTES ---

    // region --- DEBUG ---

    CROW_ROUTE(app, "/api/v1/health")
    .methods(crow::HTTPMethod::GET)
    ([]()
    {
      crow::json::wvalue res;
      res["status"] = "ok";
      res["db"] = "connected";
      return res;
    });

    // endregion

    // region --- LOGIN / REGISTER ---

    // Register new user
    // POST /api/v1/register
    CROW_ROUTE(app, "/api/v1/register")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req)
    {
      auto data = crow::json::load(req.body);
      if (!data || !data.has("username") || !data.has("password") || !data.has("email"))
        return crow::response(400, "Invalid JSON");

      dao::UserDAO user_dao;
      auto user = user_dao.Create(data["username"].s(), data["password"].s(), data["email"].s());

      if (user) return crow::response(201, "User registered");
      return crow::response(500, "Registration failed");
    });

    // Login
    // POST /api/v1/login
    CROW_ROUTE(app, "/api/v1/login")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req)
    {
      auto data = crow::json::load(req.body);
      if (!data || !data.has("username") || !data.has("password"))
        return crow::response(400, "Invalid JSON");

      services::AuthService auth{};
      auto token = auth.Login(data["username"].s(), data["password"].s());

      if (token)
      {
        crow::json::wvalue res;
        res["token"] = *token;
        return crow::response(200, res);
      }
      return crow::response(401, "Invalid credentials");
    });

    // endregion

    // endregion

    // region --- PRIVATE ROUTES ---

    // region --- DIRECTORY ---

    // List contents of a folder
    // GET /api/v1/directories?parent_id=0
    CROW_ROUTE(app, "/api/v1/directories")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");
      int user_id = ctx.user_id;

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

      return crow::response(200, res);
    });

    // Create new folder
    // POST /api/v1/directories
    CROW_ROUTE(app, "/api/v1/directories")
    .methods(crow::HTTPMethod::POST)
    ([&app](const crow::request& req)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");
      int user_id = ctx.user_id;
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

    // endregion

    // region --- REPOSITORY ---

    // Create new repo
    // POST /api/v1/repositories
    CROW_ROUTE(app, "/api/v1/repositories")
    .methods(crow::HTTPMethod::POST)
    ([&app](const crow::request& req)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");
      int user_id = ctx.user_id;
      auto data = crow::json::load(req.body);

      if (!data) return crow::response(400, "Invalid JSON");

      std::string name= data["name"].s();
      std::string description= data.has("description") ? data["description"].s() : static_cast<std::string>("");
      bool is_private = data["is_private"].b();
      bool init_readme = data.has("init_readme") ? data["init_readme"].b() : false;

      std::optional<int> dir_id;
      if (data.has("directory_id") && data["directory_id"].t() == crow::json::type::Number)
      {
        dir_id = static_cast<int>(data["directory_id"].i());
      }

      services::RepoService repo_service;
      auto result = repo_service.CreateRepository(user_id, dir_id, name, description, is_private, init_readme);

      if (result)
      {
        crow::json::wvalue res;
        res["id"] = result->id;
        res["message"] = "Repository created";
        return crow::response(201, res);
      }
      return crow::response(500, "Failed to create repository");
    });

    // endregion

    // region --- GIT OPERATIONS ---

    // Get branches
    // GET /api/v1/repositories/{id}/branches
    CROW_ROUTE(app, "/api/v1/repositories/<int>/branches")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo || repo->user_id != ctx.user_id) return crow::response(404, "Repository not found");

      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories/");
      std::string full_path = storage_path + repo->disk_path_hash + ".git";

      git::GitViewer viewer(full_path);
      auto branches = viewer.GetBranches();

      crow::json::wvalue res = crow::json::wvalue::list();
      for (size_t i = 0; i < branches.size(); i++)
      {
        res[i]["name"] = branches[i].name;
        res[i]["sha"] = branches[i].latest_commit_hash;
        res[i]["is_head"] = branches[i].is_head;
      }
      return crow::response(200, res);
    });

    // Get commits
    // GET /api/v1/repositories/{id}/commits?branch=master
    CROW_ROUTE(app, "/api/v1/repositories/<int>/commits")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo || repo->user_id != ctx.user_id) return crow::response(404, "Repository not found");

      std::string branch = req.url_params.get("branch") ? req.url_params.get("branch") : "HEAD";
      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories/");
      std::string full_path = storage_path + repo->disk_path_hash + ".git";

      git::GitViewer viewer(full_path);
      auto commits = viewer.GetCommits(branch);

      crow::json::wvalue res = crow::json::wvalue::list();
      for (size_t i = 0; i < commits.size(); i++)
      {
        res[i]["hash"] = commits[i].hash;
        res[i]["message"] = commits[i].message;
        res[i]["author"] = commits[i].author_name;
        res[i]["date"] = commits[i].timestamp;
      }

      return crow::response(200, res);
    });

    // Get tree
    // GET /api/v1/repositories/{id}/tree
    CROW_ROUTE(app, "/api/v1/repositories/<int>/tree")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo || repo->user_id != ctx.user_id) return crow::response(404, "Repository not found");

      std::string ref = req.url_params.get("ref") ? req.url_params.get("ref") : "HEAD";
      std::string path = req.url_params.get("path") ? req.url_params.get("path") : "";
      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories/");
      std::string full_path = storage_path + repo->disk_path_hash + ".git";

      git::GitViewer viewer(full_path);
      auto tree = viewer.GetTree(ref, path);

      crow::json::wvalue res = crow::json::wvalue::list();
      for (size_t i = 0; i < tree.size(); i++)
      {
        res[i]["name"] = tree[i].name;
        res[i]["type"] = tree[i].type;
        res[i]["oid"] = tree[i].oid;
      }
      return crow::response(200, res);
    });

    // Get blob
    // GET /api/v1/repositories/{id}/blob
    CROW_ROUTE(app, "/api/v1/repositories/<int>/blob")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo || repo->user_id != ctx.user_id) return crow::response(404, "Repository not found");

      std::string ref = req.url_params.get("ref") ? req.url_params.get("ref") : "HEAD";
      std::string path = req.url_params.get("path") ? req.url_params.get("path") : "";

      if (path.empty()) return crow::response(400, "Path is required for blob");

      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories/");
      std::string full_path = storage_path + repo->disk_path_hash + ".git";

      git::GitViewer viewer(full_path);
      auto content = viewer.GetBlob(ref, path);

      if (!content) return crow::response(404, "File not found in this commit");

      crow::json::wvalue res;
      res["content"] = *content;
      return crow::response(200, res);
    });

    // endregion

    // endregion
  }
}