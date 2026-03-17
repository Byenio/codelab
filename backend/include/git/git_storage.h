#ifndef CODELAB_GIT_STORAGE_H
#define CODELAB_GIT_STORAGE_H

#include <string>
#include <filesystem>
#include <vector>
#include <git2.h>
#include "models/file_entry.h"

namespace codelab::git
{
  class GitStorage
  {
  public:
    explicit GitStorage(std::string root_path) : root_path_(std::move(root_path)) {};

    bool InitRepo(const std::string& disk_path_hash, const std::string& repo_name, bool with_readme = false);
    bool DeleteRepo(const std::string& disk_path_hash);
    bool Exists(const std::string& disk_path_hash);

    std::vector<models::FileEntry> ListFiles(const std::string& disk_path_hash, const std::string& branch, const std::string& directory_path);

  private:
    std::string root_path_;

    std::string GetFullPath(const std::string& hash) const
    {
      return (std::filesystem::path(root_path_) / (hash + ".git")).string();
    }
  };
}

#endif //CODELAB_GIT_STORAGE_H