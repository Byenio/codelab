#include "api/routes.h"
#include "core/config.h"
#include "dao/repository_dao.h"
#include "dao/directory_dao.h"
#include "dao/ssh_key_dao.h"
#include "dao/pull_request_dao.h"
#include "git/git_viewer.h"
#include "services/repo_service.h"
#include "services/git_http_service.h"
#include "services/ssh_service.h"
#include <cstdio>

namespace codelab::api
{
  void RegisterRoutes(crow::App<middleware::AuthMiddleware>& app)
  {
    // region --- PUBLIC ROUTES ---

    // region --- DEBUG ---

    CROW_ROUTE(app, "/api/v1/health")
    .methods(crow::HTTPMethod::Get)
    ([]()
    {
      crow::json::wvalue res;
      res["status"] = "ok";
      res["db"] = "connected";
      return res;
    });

    // endregion

    // region --- LOGIN / REGISTER ---

    // Verify token / Get current user
    // GET /api/v1/me
    CROW_ROUTE(app, "/api/v1/me")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::UserDAO user_dao;
      auto user = user_dao.FindById(ctx.user_id);

      if (!user) return crow::response(404, "User not found");

      crow::json::wvalue res;
      res["id"] = ctx.user_id;
      res["username"] = user->username;
      res["status"] = "authenticated";

      return crow::response(200, res);
    });

    // Register new user
    // Post /api/v1/register
    CROW_ROUTE(app, "/api/v1/register")
    .methods(crow::HTTPMethod::Post)
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
    // Post /api/v1/login
    CROW_ROUTE(app, "/api/v1/login")
    .methods(crow::HTTPMethod::Post)
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

    // region --- FILESYSTEM ---

    // Resolve path to Directory or Repository
    // GET /api/v1/fs/resolve?path={project_path}
    CROW_ROUTE(app, "/api/v1/fs/resolve")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      // 1. Identify the Owner vs the Requester
      const char* user_ptr = req.url_params.get("username");
      if (!user_ptr) return crow::response(400, "Username is required");
      std::string target_username = user_ptr;

      dao::UserDAO user_dao;
      auto target_user = user_dao.FindByUsername(target_username);
      if (!target_user) return crow::response(404, "User not found");

      int owner_id = target_user->id;
      int requester_id = ctx.user_id;
      bool is_owner = (owner_id == requester_id);

      // 2. Setup Path Resolution
      std::string path_str;
      if (req.url_params.get("path")) path_str = req.url_params.get("path");

      std::vector<std::string> segments;
      std::stringstream ss(path_str);
      std::string segment;
      while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) segments.push_back(segment);
      }

      dao::DirectoryDAO dir_dao;
      dao::RepositoryDAO repo_dao;

      std::optional<int> current_parent_id = std::nullopt;
      bool is_directory = true;
      int resolved_id = 0;

      // 3. Traverse path (Using owner_id for lookups)
      for (size_t i = 0; i < segments.size(); i++) {
        const auto& name = segments[i];
        bool is_last = (i == segments.size() - 1);

        // Look for a directory owned by the target user
        auto dir = dir_dao.FindByName(owner_id, current_parent_id, name);
        if (dir) {
          current_parent_id = dir->id;
          if (is_last) resolved_id = dir->id;
          continue;
        }

        if (is_last) {
           // Look for a repository owned by the target user
           auto repo = repo_dao.FindByName(owner_id, current_parent_id, name);
           if (repo) {
             // PRIVACY CHECK: Direct access via URL
             if (repo->is_private && !is_owner && !repo_dao.IsCollaborator(repo->id, requester_id)) {
                return crow::response(403, "Access Denied: This repository is private");
             }
             resolved_id = repo->id;
             is_directory = false;
             break;
           }
        }

        return crow::response(404, "Path not found");
      }

      // 4. Build Response
      crow::json::wvalue res;
      if (is_directory) {
        res["type"] = "directory";
        if (current_parent_id.has_value()) res["directory_id"] = current_parent_id.value();

        // Use owner_id to list the contents
        auto dirs = dir_dao.ListByParent(owner_id, current_parent_id);
        auto repos = repo_dao.ListByDirectory(owner_id, current_parent_id);

        res["directories"] = crow::json::wvalue::list();
        for(size_t i=0; i<dirs.size(); i++) {
           res["directories"][i]["id"] = dirs[i].id;
           res["directories"][i]["name"] = dirs[i].name;
           res["directories"][i]["type"] = "directory";
        }

        res["repositories"] = crow::json::wvalue::list();
        size_t repo_idx = 0;
        for(size_t i=0; i<repos.size(); i++) {
           // PRIVACY FILTER: Skip private repos if the requester isn't the owner or collaborator
           if (repos[i].is_private && !is_owner && !repo_dao.IsCollaborator(repos[i].id, requester_id)) {
              continue;
           }

           res["repositories"][repo_idx]["id"] = repos[i].id;
           res["repositories"][repo_idx]["name"] = repos[i].name;
           res["repositories"][repo_idx]["type"] = "repository";
           res["repositories"][repo_idx]["is_private"] = repos[i].is_private;
           repo_idx++;
        }
      } else {
        // REPOSITORY VIEW
        res["type"] = "repository";
        if (current_parent_id.has_value()) res["directory_id"] = current_parent_id.value();

        auto repo = repo_dao.FindById(resolved_id);
        if (repo) {
            // Double check privacy even if FindByName caught it
            if (repo->is_private && !is_owner && !repo_dao.IsCollaborator(repo->id, requester_id)) {
                return crow::response(403, "Access Denied");
            }
            res["repository"]["id"] = repo->id;
            res["repository"]["name"] = repo->name;
            res["repository"]["description"] = repo->description; // Preserved
            res["repository"]["is_private"] = repo->is_private;
        }
      }

      return crow::response(200, res);
    });

    // endregion

    // region --- DIRECTORY ---

    // List contents of a folder
    // Get /api/v1/directories?parent_id=0
    CROW_ROUTE(app, "/api/v1/directories")
    .methods(crow::HTTPMethod::Get)
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
    // Post /api/v1/directories
    CROW_ROUTE(app, "/api/v1/directories")
    .methods(crow::HTTPMethod::Post)
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

    // Delete folder
    // DELETE /api/v1/directories/<int>
    CROW_ROUTE(app, "/api/v1/directories/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&app](const crow::request& req, int directory_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      if (repo_service.DeleteDirectory(ctx.user_id, directory_id)) {
          return crow::response(200, "Directory deleted");
      }
      return crow::response(400, "Failed to delete directory");
    });

    // Update (Move/Rename) folder
    // PATCH /api/v1/directories/<int>
    CROW_ROUTE(app, "/api/v1/directories/<int>")
    .methods(crow::HTTPMethod::Patch)
    ([&app](const crow::request& req, int directory_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401, "Unauthorized");

      auto data = crow::json::load(req.body);
      if (!data) return crow::response(400, "Invalid JSON");

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      bool success = true;

      // Handle rename
      if (data.has("name")) {
         if (!repo_service.RenameDirectory(ctx.user_id, directory_id, data["name"].s())) {
             success = false;
         }
      }

      // Handle move
      if (data.has("parent_id")) {
         std::optional<int> new_parent_id = std::nullopt;
         if (data["parent_id"].t() == crow::json::type::Number) {
             new_parent_id = data["parent_id"].i();
         }

         if (!repo_service.MoveDirectory(ctx.user_id, directory_id, new_parent_id)) {
             success = false;
         }
      }

      if (success) {
          return crow::response(200, "Directory updated");
      }
      return crow::response(400, "Failed to completely update directory");
    });

    // endregion

    // region --- REPOSITORY ---

    // Delete repository
    // DELETE /api/v1/repositories/<int>
    CROW_ROUTE(app, "/api/v1/repositories/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&app](const crow::request& req, int repo_id){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      if (repo_service.DeleteRepository(ctx.user_id, repo_id)) {
        return crow::response(200, "Repository deleted");
      }
      return crow::response(400, "Failed to delete repository");
    });

    // Update (Move) repository
    // PATCH /api/v1/repositories/<int>
    CROW_ROUTE(app, "/api/v1/repositories/<int>")
    .methods(crow::HTTPMethod::Patch)
    ([&app](const crow::request& req, int repo_id){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      auto data = crow::json::load(req.body);
      if (!data) return crow::response(400, "Invalid JSON");

      if (!data.has("directory_id")) {
          return crow::response(400, "Missing directory_id parameter");
      }

      std::optional<int> new_dir_id = std::nullopt;
      if (data["directory_id"].t() == crow::json::type::Number) {
         new_dir_id = data["directory_id"].i();
      } else if (data["directory_id"].t() != crow::json::type::Null) {
         return crow::response(400, "directory_id must be a number or null");
      }

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      if (repo_service.MoveRepository(ctx.user_id, repo_id, new_dir_id)) {
        return crow::response(200, "Repository moved");
      }
      return crow::response(400, "Failed to move repository");
    });

    // Create new repo
    // Post /api/v1/repositories
    CROW_ROUTE(app, "/api/v1/repositories")
    .methods(crow::HTTPMethod::Post)
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

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

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

    // List user repositories
    // GET /api/v1/user/repositories
    CROW_ROUTE(app, "/api/v1/user/repositories")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::RepositoryDAO dao;
      auto repos = dao.ListByUser(ctx.user_id);

      crow::json::wvalue res = crow::json::wvalue::list();
      for (size_t i = 0; i < repos.size(); i++) {
        res[i]["id"] = repos[i].id;
        res[i]["name"] = repos[i].name;
        res[i]["description"] = repos[i].description;
        res[i]["is_private"] = repos[i].is_private;
        res[i]["created_at"] = repos[i].created_at;
      }
      return crow::response(200, res);
    });

    // Get repository details
    // GET /api/v1/repositories/<repo_name>?directory_id=1
    CROW_ROUTE(app, "/api/v1/repositories/<string>")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, const std::string& repo_name){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      std::optional<int> dir_id = std::nullopt;
      if (req.url_params.get("directory_id"))
      {
        try
        {
          dir_id = std::stoi(req.url_params.get("directory_id"));
        } catch (...) {}
      }

      dao::RepositoryDAO dao;
      auto repo = dao.FindByName(ctx.user_id, dir_id, repo_name);

      if (!repo) {
          return crow::response(404, "Repository not found");
      }

      crow::json::wvalue res;
      res["id"] = repo->id;
      res["name"] = repo->name;
      res["description"] = repo->description;
      res["is_private"] = repo->is_private;
      res["created_at"] = repo->created_at;

      return crow::response(200, res);
    });

    // Get repository file tree
    // GET /api/v1/repositories/<repo_name>/tree?directory_id=1&branch=master&path=src/
    CROW_ROUTE(app, "/api/v1/repositories/<string>/tree")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, const std::string& repo_name)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      std::string branch = req.url_params.get("branch") ? req.url_params.get("branch") : "master";
      std::string path = req.url_params.get("path") ? req.url_params.get("path") : "";

      std::optional<int> dir_id = std::nullopt;
      if (req.url_params.get("directory_id")) {
        try {
           dir_id = std::stoi(req.url_params.get("directory_id"));
        } catch(...) {}
      }

      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService service(storage_path);

      auto files = service.GetFileTree(ctx.user_id, dir_id, repo_name, branch, path);

      crow::json::wvalue res = crow::json::wvalue::list();
      for (size_t i = 0; i < files.size(); i++)
      {
        res[i]["name"] = files[i].name;
        res[i]["path"] = files[i].path;
        res[i]["is_directory"] = files[i].is_directory;
        res[i]["size"] = files[i].size;
      }

      return crow::response(200, res);
    });

    // Get file content
    // GET /api/v1/repositories/<repo>/blob?directory_id=1&path=src/main.cc
    CROW_ROUTE(app, "/api/v1/repositories/<string>/blob")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, std::string repo_name){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      std::string branch = req.url_params.get("branch") ? req.url_params.get("branch") : "master";
      std::string path = req.url_params.get("path") ? req.url_params.get("path") : "";

      std::optional<int> dir_id = std::nullopt;
      if (req.url_params.get("directory_id")) {
        try {
           dir_id = std::stoi(req.url_params.get("directory_id"));
        } catch(...) {}
      }

      if (path.empty()) return crow::response(400, "Path is required");

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService service(storage_path);

      auto content = service.GetFileContent(ctx.user_id, dir_id, repo_name, branch, path);

      if (content) {
          crow::json::wvalue res;
          res["content"] = *content;
          return crow::response(200, res);
      }
      return crow::response(404, "File not found");
    });

    // Get collaborators
    // GET /api/v1/repositories/<int>/collaborators
    CROW_ROUTE(app, "/api/v1/repositories/<int>/collaborators")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, int repo_id){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      dao::UserDAO user_dao;
      auto collab_ids = repo_dao.ListCollaboratorIds(repo_id);

      crow::json::wvalue res = crow::json::wvalue::list();
      int i = 0;
      for (int cid : collab_ids) {
        auto u = user_dao.FindById(cid);
        if (u) {
          res[i]["id"] = u->id;
          res[i]["username"] = u->username;
          i++;
        }
      }
      return crow::response(200, res);
    });

    // Add collaborator
    // POST /api/v1/repositories/<int>/collaborators
    CROW_ROUTE(app, "/api/v1/repositories/<int>/collaborators")
    .methods(crow::HTTPMethod::Post)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      auto data = crow::json::load(req.body);
      if (!data || !data.has("username")) return crow::response(400, "username required");
      std::string target_username = data["username"].s();

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      dao::UserDAO user_dao;
      auto target_user = user_dao.FindByUsername(target_username);
      if (!target_user) return crow::response(404, "Target user not found");

      if (target_user->id == repo->user_id) return crow::response(400, "User is the owner");

      if (repo_dao.AddCollaborator(repo_id, target_user->id)) {
         return crow::response(200, "Collaborator added");
      }
      return crow::response(500, "Failed to add collaborator");
    });

    // Remove collaborator
    // DELETE /api/v1/repositories/<int>/collaborators/<string>
    CROW_ROUTE(app, "/api/v1/repositories/<int>/collaborators/<string>")
    .methods(crow::HTTPMethod::Delete)
    ([&app](const crow::request& req, int repo_id, const std::string& target_username)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      dao::UserDAO user_dao;
      auto target_user = user_dao.FindByUsername(target_username);
      if (!target_user) return crow::response(404, "Target user not found");

      if (repo_dao.RemoveCollaborator(repo_id, target_user->id)) {
         return crow::response(200, "Collaborator removed");
      }
      return crow::response(500, "Failed to remove collaborator");
    });


    // region --- PULL REQUESTS ---

    // List PRs
    CROW_ROUTE(app, "/api/v1/repositories/<int>/pull-requests")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      bool authorized = !repo->is_private;
      if (!authorized && ctx.user_id != 0) {
        if (repo->user_id == ctx.user_id || repo_dao.IsCollaborator(repo->id, ctx.user_id)) authorized = true;
      }
      if (!authorized) return crow::response(404, "Repository not found");

      dao::PullRequestDAO pr_dao;
      auto prs = pr_dao.ListByRepository(repo_id);

      crow::json::wvalue res = crow::json::wvalue::list();
      int i = 0;
      for (const auto& pr : prs) {
        res[i]["id"] = pr.id;
        res[i]["author_id"] = pr.author_id;
        res[i]["title"] = pr.title;
        res[i]["description"] = pr.description;
        res[i]["source_branch"] = pr.source_branch;
        res[i]["target_branch"] = pr.target_branch;
        res[i]["status"] = pr.status;
        res[i]["created_at"] = pr.created_at;
        i++;
      }
      return crow::response(200, res);
    });

    // Create PR
    CROW_ROUTE(app, "/api/v1/repositories/<int>/pull-requests")
    .methods(crow::HTTPMethod::Post)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      auto data = crow::json::load(req.body);
      if (!data || !data.has("title") || !data.has("source_branch") || !data.has("target_branch"))
        return crow::response(400, "Invalid JSON");

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      if (!repo_dao.IsCollaborator(repo_id, ctx.user_id) && repo->user_id != ctx.user_id) {
         return crow::response(403, "Must be a collaborator to open PR");
      }

      models::PullRequest pr;
      pr.repository_id = repo_id;
      pr.author_id = ctx.user_id;
      pr.title = data["title"].s();
      pr.description = data.has("description") ? (std::string)data["description"].s() : "";
      pr.source_branch = data["source_branch"].s();
      pr.target_branch = data["target_branch"].s();
      pr.status = "open";

      dao::PullRequestDAO pr_dao;
      auto id = pr_dao.Create(pr);
      if (id) {
        crow::json::wvalue res;
        res["id"] = *id;
        return crow::response(201, res);
      }
      return crow::response(500, "Failed to create PR");
    });

    // Merge PR
    CROW_ROUTE(app, "/api/v1/repositories/<int>/pull-requests/<int>/merge")
    .methods(crow::HTTPMethod::Post)
    ([&app](const crow::request& req, int repo_id, int pr_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404);

      if (!repo_dao.IsCollaborator(repo_id, ctx.user_id) && repo->user_id != ctx.user_id) {
         return crow::response(403, "Must be a collaborator to merge PR");
      }

      dao::PullRequestDAO pr_dao;
      auto pr = pr_dao.FindById(pr_id);
      if (!pr || pr->repository_id != repo_id) return crow::response(404);
      if (pr->status != "open") return crow::response(400, "PR is not open");

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      if (repo_service.MergeBranch(ctx.user_id, repo_id, pr->source_branch, pr->target_branch)) {
        pr_dao.UpdateStatus(pr_id, "merged");
        return crow::response(200, "PR Merged");
      }
      return crow::response(400, "Failed to merge PR");
    });

    // Close PR
    CROW_ROUTE(app, "/api/v1/repositories/<int>/pull-requests/<int>/close")
    .methods(crow::HTTPMethod::Post)
    ([&app](const crow::request& req, int repo_id, int pr_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404);

      if (!repo_dao.IsCollaborator(repo_id, ctx.user_id) && repo->user_id != ctx.user_id) {
         return crow::response(403, "Must be a collaborator to close PR");
      }

      dao::PullRequestDAO pr_dao;
      auto pr = pr_dao.FindById(pr_id);
      if (!pr || pr->repository_id != repo_id) return crow::response(404);
      if (pr->status != "open") return crow::response(400, "PR is not open");

      if (pr_dao.UpdateStatus(pr_id, "closed")) {
        return crow::response(200, "PR Closed");
      }
      return crow::response(500, "Failed to close PR");
    });

    // Get PR Diff
    CROW_ROUTE(app, "/api/v1/repositories/<int>/pull-requests/<int>/diff")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, int repo_id, int pr_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      bool authorized = !repo->is_private;
      if (!authorized && ctx.user_id != 0) {
        if (repo->user_id == ctx.user_id || repo_dao.IsCollaborator(repo->id, ctx.user_id)) authorized = true;
      }
      if (!authorized) return crow::response(404, "Repository not found");

      dao::PullRequestDAO pr_dao;
      auto pr = pr_dao.FindById(pr_id);
      if (!pr || pr->repository_id != repo_id) return crow::response(404, "Pull request not found");

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      std::string repo_dir = storage_path + repo->disk_path_hash + ".git";

      if (!std::filesystem::exists(repo_dir)) {
        crow::json::wvalue res;
        res["diff"] = "Backend Error: Repository directory does not exist at path: " + repo_dir +
                      "\nPlease check your REPO_STORAGE_PATH configuration relative to where the server binary is run.";
        return crow::response(200, res);
      }

      std::string cmd = "cd " + repo_dir + " && git diff '" + pr->target_branch + "'...'" + pr->source_branch + "' 2>&1";

      std::string diff_output = "";
      char buffer[128];
      FILE* pipe = popen(cmd.c_str(), "r");
      if (!pipe) {
        return crow::response(500, "Failed to execute git command");
      }

      while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        diff_output += buffer;
      }
      pclose(pipe);

      crow::json::wvalue res;
      res["diff"] = diff_output;
      return crow::response(200, res);
    });

    // endregion

    // endregion

    // region --- GIT OPERATIONS ---

    // Get branches
    // Get /api/v1/repositories/{id}/branches
    CROW_ROUTE(app, "/api/v1/repositories/<int>/branches")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, int repo_id)
    {
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);

      bool authorized = !repo->is_private;
      if (!authorized && ctx.user_id != 0)
      {
        if (repo->user_id == ctx.user_id || repo_dao.IsCollaborator(repo->id, ctx.user_id)) authorized = true;
      }

      if (!authorized) return crow::response(404, "Repository not found");

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
    // Get /api/v1/repositories/{id}/commits?branch=master
    CROW_ROUTE(app, "/api/v1/repositories/<int>/commits")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, int repo_id)
    {
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);

      bool authorized = !repo->is_private;
      if (!authorized && ctx.user_id != 0)
      {
        if (repo->user_id == ctx.user_id || repo_dao.IsCollaborator(repo->id, ctx.user_id)) authorized = true;
      }

      if (!authorized) return crow::response(404, "Repository not found");

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
    // Get /api/v1/repositories/{id}/tree
    CROW_ROUTE(app, "/api/v1/repositories/<int>/tree")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, int repo_id)
    {
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);

      bool authorized = !repo->is_private;
      if (!authorized && ctx.user_id != 0)
      {
        if (repo->user_id == ctx.user_id || repo_dao.IsCollaborator(repo->id, ctx.user_id)) authorized = true;
      }

      if (!authorized) return crow::response(404, "Repository not found");

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
    // Get /api/v1/repositories/{id}/blob
    CROW_ROUTE(app, "/api/v1/repositories/<int>/blob")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, int repo_id)
    {
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404, "Repository not found");

      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);

      bool authorized = !repo->is_private;
      if (!authorized && ctx.user_id != 0)
      {
        if (repo->user_id == ctx.user_id || repo_dao.IsCollaborator(repo->id, ctx.user_id)) authorized = true;
      }

      if (!authorized) return crow::response(404, "Repository not found");

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

    // region --- BRANCH MANAGEMENT ---

    // Create branch
    // POST /api/v1/repositories/<int>/branches
    CROW_ROUTE(app, "/api/v1/repositories/<int>/branches")
    .methods(crow::HTTPMethod::Post)
    ([&app](const crow::request& req, int repo_id)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      auto data = crow::json::load(req.body);
      if (!data) return crow::response(400, "Invalid JSON");
      if (!data.has("name")) return crow::response(400, "Branch name is required");

      std::string branch_name = data["name"].s();
      std::string target_branch = data.has("target_branch") ? (std::string)data["target_branch"].s() : "HEAD";

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      if (repo_service.CreateBranch(ctx.user_id, repo_id, branch_name, target_branch)) {
        return crow::response(201, "Branch created");
      }
      return crow::response(400, "Failed to create branch");
    });

    // Delete branch
    // DELETE /api/v1/repositories/<int>/branches/<string>
    CROW_ROUTE(app, "/api/v1/repositories/<int>/branches/<string>")
    .methods(crow::HTTPMethod::Delete)
    ([&app](const crow::request& req, int repo_id, const std::string& branch_name)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      if (repo_service.DeleteBranch(ctx.user_id, repo_id, branch_name)) {
        return crow::response(200, "Branch deleted");
      }
      return crow::response(400, "Failed to delete branch");
    });

    // Merge branch
    // POST /api/v1/repositories/<int>/branches/<string>/merge
    CROW_ROUTE(app, "/api/v1/repositories/<int>/branches/<string>/merge")
    .methods(crow::HTTPMethod::Post)
    ([&app](const crow::request& req, int repo_id, const std::string& target_branch)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      auto data = crow::json::load(req.body);
      if (!data || !data.has("source_branch")) return crow::response(400, "source_branch is required");

      std::string source_branch = data["source_branch"].s();

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService repo_service(storage_path);

      if (repo_service.MergeBranch(ctx.user_id, repo_id, source_branch, target_branch)) {
        return crow::response(200, "Branch merged");
      }
      return crow::response(400, "Failed to merge branch/conflicts detected");
    });

    // endregion

    // region --- GIT SMART HTTP ---

    // Handshake
    // Get /git/{repo_id}.git/info/refs?service=git-upload-pack
    CROW_ROUTE(app, "/git/<int>.git/info/refs")
    .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req, int repo_id){
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404);

      std::string service = req.url_params.get("service");
      if (service.empty()) return crow::response(400, "Service required");

      bool is_read_op = (service == "git-upload-pack");
      bool is_write_op = (service == "git-receive-pack");

      int user_id = 0;
      std::string auth_header = req.get_header_value("Authorization");
      services::AuthService auth {};

      auto auth_result = auth.VerifyBasicAuth(auth_header);
      if (auth_result) user_id = *auth_result;

      bool authorized = false;

      if (is_read_op)
      {
        if ((!repo->is_private) || (user_id != 0 && (repo->user_id == user_id || repo_dao.IsCollaborator(repo->id, user_id)))) authorized = true;
      } else if (is_write_op)
      {
        if (user_id != 0 && (repo->user_id == user_id || repo_dao.IsCollaborator(repo->id, user_id))) authorized = true;
      }

      if (!authorized) {
        crow::response res(401);
        res.set_header("WWW-Authenticate", "Basic realm=\"Codelab git\"");
        return res;
      }

      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories/");
      std::string full_path = storage_path + repo->disk_path_hash + ".git";

      services::GitHttpService git_service;
      auto output = git_service.GetInfoRefs(full_path, service);

      if (!output) return crow::response(500, "Git error");

      crow::response res(*output);
      res.set_header("Content-Type", "application/x-" + service + "-advertisement");
      res.set_header("Cache-Control", "no-cache");
      return res;
    });

    // Data transfer
    // Post /git/{repo_id}.git/git-upload-pack
    // Post /git/{repo_id}.git/git-receive-pack
    CROW_ROUTE(app, "/git/<int>.git/<string>")
    .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req, int repo_id, const std::string& service){
      dao::RepositoryDAO repo_dao;
      auto repo = repo_dao.FindById(repo_id);
      if (!repo) return crow::response(404);

      if (service != "git-upload-pack" && service != "git-receive-pack")
      {
        return crow::response(400, "Invalid service");
      }

      bool is_read_op = (service == "git-upload-pack");
      bool is_write_op = (service == "git-receive-pack");

      int user_id = 0;
      std::string auth_header = req.get_header_value("Authorization");
      services::AuthService auth {};
      auto auth_result = auth.VerifyBasicAuth(auth_header);
      if (auth_result) user_id = *auth_result;

      bool authorized = false;

      if (is_read_op)
      {
        if ((!repo->is_private) || (user_id != 0 && (repo->user_id == user_id || repo_dao.IsCollaborator(repo->id, user_id)))) authorized = true;
      } else if (is_write_op)
      {
        if (user_id != 0 && (repo->user_id == user_id || repo_dao.IsCollaborator(repo->id, user_id))) authorized = true;
      }

      if (!authorized) {
        crow::response res(401);
        res.set_header("WWW-Authenticate", "Basic realm=\"Codelab git\"");
        return res;
      }

      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories/");
      std::string full_path = storage_path + repo->disk_path_hash + ".git";

      services::GitHttpService git_service;
      auto output = git_service.HandleRpc(full_path, service, req.body);

      if (!output) return crow::response(500, "Git error");

      crow::response res(*output);
      res.set_header("Content-Type", "application/x-" + service + "-result");
      res.set_header("Cache-Control", "no-cache");
      return res;
    });

    // endregion

    // region --- SSH ---

    // List SSH keys
    // Get /api/v1/user/keys
    CROW_ROUTE(app, "/api/v1/user/keys")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::SSHKeyDAO ssh_dao;
      auto keys = ssh_dao.ListByUser(ctx.user_id);

      crow::json::wvalue res = crow::json::wvalue::list();
      for (size_t i = 0; i < keys.size(); i++)
      {
        res[i]["id"] = keys[i].id;
        res[i]["title"] = keys[i].title;
        res[i]["key"] = keys[i].key_content;
        res[i]["created_at"] = keys[i].created_at;
      }

      return crow::response(200, res);
    });

    // Add SSH key
    // Post /api/v1/user/keys
    CROW_ROUTE(app, "/api/v1/user/keys")
    .methods(crow::HTTPMethod::Post)
    ([&app](const crow::request& req)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      auto data = crow::json::load(req.body);
      if (!data || !data.has("title") || !data.has("key"))
      {
        return crow::response(400, "Invalid JSON - title and key required");
      }

      std::string title = data["title"].s();
      std::string key = data["key"].s();

      services::SSHService ssh_service;
      auto id = ssh_service.AddKey(ctx.user_id, title, key);

      if (id)
      {
        crow::json::wvalue res;
        res["id"] = *id;
        res["message"] = "Key added";
        return crow::response(200, res);
      }

      return crow::response(400, "Invalid key format");
    });

    // Delete SSH key
    // DELETE /api/v1/user/keys/<id>
    CROW_ROUTE(app, "/api/v1/user/keys/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&app](const crow::request& req, int key_id){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      services::SSHService service;
      bool success = service.RemoveKey(key_id, ctx.user_id);

      if (success) return crow::response(200, "Key deleted");
      return crow::response(404, "Key not found");
    });

    // endregion

    // endregion
  }
}
