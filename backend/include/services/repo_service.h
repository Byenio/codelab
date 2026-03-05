#ifndef CODELAB_REPO_SERVICE_H
#define CODELAB_REPO_SERVICE_H

#include "dao/directory_dao.h"
#include "dao/repository_dao.h"
#include "git/git_storage.h"
#include "models/respository.h"

namespace codelab::services
{
  class RepoService
  {
  public:
    RepoService();

    std::optional<models::Repository> CreateRepository (
      int user_id,
      std::optional<int> parent_id,
      const std::string& name,
      const std::string& description,
      bool is_private
    );

  private:
    dao::RepositoryDAO repo_dao_;
    dao::DirectoryDAO dir_dao_;
    git::GitStorage storage_;

    std::string GenerateDiskHash();
  };
}

#endif //CODELAB_REPO_SERVICE_H