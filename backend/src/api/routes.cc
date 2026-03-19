#include "api/routes.h"
#include "core/config.h"
#include "dao/repository_dao.h"
#include "dao/directory_dao.h"
#include "dao/ssh_key_dao.h"
#include "git/git_viewer.h"
#include "services/repo_service.h"
#include "services/git_http_service.h"
#include "services/ssh_service.h"

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

    // endregion

    // region --- REPOSITORY ---

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
    // GET /api/v1/repositories/<repo_name>
    CROW_ROUTE(app, "/api/v1/repositories/<string>")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, const std::string& repo_name){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      dao::RepositoryDAO dao;
      auto repo = dao.FindByName(ctx.user_id, std::nullopt, repo_name);

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
    // GET /api/v1/repositories/<repo_name>/tree?branch=master&path=src/
    CROW_ROUTE(app, "/api/v1/repositories/<string>/tree")
    .methods(crow::HTTPMethod::Get)
    ([&app](const crow::request& req, const std::string& repo_name)
    {
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      std::string branch = req.url_params.get("branch") ? req.url_params.get("branch") : "master";
      std::string path = req.url_params.get("path") ? req.url_params.get("path") : "";

      std::string storage_path = core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService service(storage_path);

      auto files = service.GetFileTree(ctx.user_id, repo_name, branch, path);

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
    // GET /api/v1/repositories/<repo>/blob?path=src/main.cc
    CROW_ROUTE(app, "/api/v1/repositories/<string>/blob")
    .methods(crow::HTTPMethod::GET)
    ([&app](const crow::request& req, std::string repo_name){
      auto& ctx = app.get_context<middleware::AuthMiddleware>(req);
      if (ctx.user_id == 0) return crow::response(401);

      std::string branch = req.url_params.get("branch") ? req.url_params.get("branch") : "master";
      std::string path = req.url_params.get("path") ? req.url_params.get("path") : "";

      if (path.empty()) return crow::response(400, "Path is required");

      std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories");
      services::RepoService service(storage_path);

      auto content = service.GetFileContent(ctx.user_id, repo_name, branch, path);

      if (content) {
          crow::json::wvalue res;
          res["content"] = *content;
          return crow::response(200, res);
      }
      return crow::response(404, "File not found");
    });

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
        // TODO: allow collaborators
        if (repo->user_id == ctx.user_id) authorized = true;
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
        // TODO: allow collaborators
        if (repo->user_id == ctx.user_id) authorized = true;
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
        // TODO: allow collaborators
        if (repo->user_id == ctx.user_id) authorized = true;
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
        // TODO: allow collaborators
        if (repo->user_id == ctx.user_id) authorized = true;
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
        if ((!repo->is_private) || (user_id != 0 && repo->user_id == user_id)) authorized = true;
      } else if (is_write_op)
      {
        // TODO: allow collaborators
        if (user_id != 0 && repo->user_id == user_id) authorized = true;
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
        if ((!repo->is_private) || (user_id != 0 && repo->user_id == user_id)) authorized = true;
      } else if (is_write_op)
      {
        // TODO: allow collaborators
        if (user_id != 0 && repo->user_id == user_id) authorized = true;
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
