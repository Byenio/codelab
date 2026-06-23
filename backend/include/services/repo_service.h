#ifndef CODELAB_REPO_SERVICE_H
#define CODELAB_REPO_SERVICE_H

#include "dao/directory_dao.h"
#include "dao/repository_dao.h"
#include "git/git_storage.h"
#include "models/file_entry.h"
#include "models/respository.h"

namespace codelab::services
{
  class RepoService
  {
  public:
    explicit RepoService(std::string root_path = "../../data/repositories");

    std::optional<models::Repository> CreateRepository (
      int user_id,
      std::optional<int> parent_id,
      const std::string& name,
      const std::string& description,
      bool is_private,
      bool init_readme = false
    );

    std::vector<models::FileEntry> GetFileTree(int user_id, std::optional<int> directory_id, const std::string& repo_name, const std::string& branch, const std::string& path);
    std::optional<std::string> GetFileContent(int user_id, std::optional<int> directory_id, const std::string& repo_name, const std::string& branch, const std::string& filepath);

    bool DeleteRepository(int user_id, int repo_id);
    bool MoveRepository(int user_id, int repo_id, std::optional<int> new_directory_id);

    bool DeleteDirectory(int user_id, int directory_id);
    bool MoveDirectory(int user_id, int directory_id, std::optional<int> parent_id);
    bool RenameDirectory(int user_id, int directory_id, const std::string& name);

    bool CreateBranch(int user_id, int repo_id, const std::string& branch_name, const std::string& base_branch);
    bool DeleteBranch(int user_id, int repo_id, const std::string& branch_name);
    bool MergeBranch(int user_id, int repo_id, const std::string& source_branch, const std::string& target_branch);

  private:
    dao::RepositoryDAO repo_dao_;
    dao::DirectoryDAO dir_dao_;
    git::GitStorage storage_;

    std::string GenerateDiskHash();
  };
}

#endif //CODELAB_REPO_SERVICE_H