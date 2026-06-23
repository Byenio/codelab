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

  bool RepoService::DeleteRepository(int user_id, int repo_id)
  {
    auto repo = repo_dao_.FindById(repo_id);
    if (!repo || repo->user_id != user_id) return false;

    // Delete from DB
    if (!repo_dao_.Delete(repo_id)) return false;

    // Delete from disk
    storage_.DeleteRepo(repo->disk_path_hash);
    return true;
  }

  bool RepoService::MoveRepository(int user_id, int repo_id, std::optional<int> new_directory_id)
  {
    auto repo = repo_dao_.FindById(repo_id);
    if (!repo || repo->user_id != user_id) return false;

    // Validate new directory
    if (new_directory_id.has_value()) {
      auto directory = dir_dao_.FindById(new_directory_id.value());
      if (!directory || directory->user_id != user_id) {
        std::cerr << "[!] New directory not found or permission denied" << std::endl;
        return false;
      }
    }

    // Check if a repo with the same name already exists in the target directory
    auto existing = repo_dao_.FindByName(user_id, new_directory_id, repo->name);
    if (existing.has_value()) {
        std::cerr << "[!] A repository with this name already exists in the target folder" << std::endl;
        return false;
    }

    return repo_dao_.UpdateDirectory(repo_id, new_directory_id);
  }

  bool RepoService::DeleteDirectory(int user_id, int directory_id)
  {
    auto dir = dir_dao_.FindById(directory_id);
    if (!dir || dir->user_id != user_id) return false;

    // Recursively delete children
    auto children = dir_dao_.ListByParent(user_id, directory_id);
    for (auto c : children) {
      DeleteDirectory(user_id, c.id);
    }

    // Delete repos from disk
    auto repos = repo_dao_.ListByDirectory(user_id, directory_id);
    for (auto r : repos) {
      storage_.DeleteRepo(r.disk_path_hash);
    }

    return dir_dao_.Delete(directory_id);
  }

  bool RepoService::MoveDirectory(int user_id, int directory_id, std::optional<int> parent_id)
  {
    auto dir = dir_dao_.FindById(directory_id);
    if (!dir || dir->user_id != user_id) return false;

    // Prevent cycles if parent_id is set
    if (parent_id.has_value()) {
       int current_check = parent_id.value();
       while (true) {
         if (current_check == directory_id) return false; // Cycle detected
         auto parent_dir = dir_dao_.FindById(current_check);
         if (!parent_dir || !parent_dir->parent_id.has_value()) break;
         current_check = parent_dir->parent_id.value();
       }

       auto parent = dir_dao_.FindById(parent_id.value());
       if (!parent || parent->user_id != user_id) return false;
    }

    // Check collision
    auto existing = dir_dao_.FindByName(user_id, parent_id, dir->name);
    if (existing.has_value()) return false;

    return dir_dao_.UpdateParent(directory_id, parent_id);
  }

  bool RepoService::RenameDirectory(int user_id, int directory_id, const std::string& name)
  {
    auto dir = dir_dao_.FindById(directory_id);
    if (!dir || dir->user_id != user_id) return false;

    auto existing = dir_dao_.FindByName(user_id, dir->parent_id, name);
    if (existing.has_value()) return false;

    return dir_dao_.UpdateName(directory_id, name);
  }

  bool RepoService::CreateBranch(int user_id, int repo_id, const std::string& branch_name, const std::string& base_branch)
  {
    auto repo = repo_dao_.FindById(repo_id);
    if (!repo || repo->user_id != user_id) return false;
    return storage_.CreateBranch(repo->disk_path_hash, branch_name, base_branch);
  }

  bool RepoService::DeleteBranch(int user_id, int repo_id, const std::string& branch_name)
  {
    auto repo = repo_dao_.FindById(repo_id);
    if (!repo || repo->user_id != user_id) return false;
    return storage_.DeleteBranch(repo->disk_path_hash, branch_name);
  }

  bool RepoService::MergeBranch(int user_id, int repo_id, const std::string& source_branch, const std::string& target_branch)
  {
    auto repo = repo_dao_.FindById(repo_id);
    if (!repo || repo->user_id != user_id) return false;
    return storage_.MergeBranch(repo->disk_path_hash, source_branch, target_branch);
  }
}
