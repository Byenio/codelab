#include "services/repo_service.h"
#include <random>
#include <sstream>
#include <iostream>

namespace codelab::services
{
  RepoService::RepoService(std::string root_path) : storage_(std::move(root_path)) {}

  std::string RepoService::GenerateDiskHash()
  {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    for (int i = 0; i < 32; ++i)
    {
      ss << std::hex << dis(gen);
    }

    return ss.str();
  }

  std::optional<models::Repository> RepoService::CreateRepository(int user_id, std::optional<int> parent_id, const std::string &name, const std::string &description, bool is_private, bool init_readme)
  {
    if (parent_id.has_value())
    {
      auto directory = dir_dao_.FindById(*parent_id);

      if (!directory || directory->user_id != user_id)
      {
        std::cerr << "[!] Directory not found" << std::endl;
        return std::nullopt;
      }
    }

    auto existing = repo_dao_.FindByName(user_id, parent_id, name);
    if (existing.has_value())
    {
      std::cerr << "[!] Repo with this name already exists in this folder" << std::endl;
      return std::nullopt;
    }

    models::Repository repo;
    repo.user_id = user_id;
    repo.directory_id = parent_id;
    repo.name = name;
    repo.description = description;
    repo.is_private = is_private;
    repo.disk_path_hash = GenerateDiskHash();

    if (!storage_.InitRepo(repo.disk_path_hash, name, init_readme))
    {
      return std::nullopt;
    }

    auto id = repo_dao_.Create(repo);
    if (!id.has_value())
    {
      storage_.DeleteRepo(repo.disk_path_hash);
      return std::nullopt;
    }

    repo.id = id.value();
    return repo;
  }

  std::vector<models::FileEntry> RepoService::GetFileTree(int user_id, std::optional<int> directory_id, const std::string &repo_name, const std::string &branch, const std::string &path)
  {
    auto repo = repo_dao_.FindByName(user_id, directory_id, repo_name);

    if (!repo) return {};

    return storage_.ListFiles(repo->disk_path_hash, branch, path);
  }

  std::optional<std::string> RepoService::GetFileContent(int user_id, std::optional<int> directory_id, const std::string &repo_name, const std::string &branch, const std::string &filepath)
  {
    auto repo = repo_dao_.FindByName(user_id, directory_id, repo_name);
    if (!repo) return std::nullopt;
    return storage_.GetFileContent(repo->disk_path_hash, branch, filepath);
  }
}
