#include "git/git_storage.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace codelab::git
{
  bool GitStorage::InitRepo(const std::string &disk_path_hash)
  {
    std::string full_path = GetFullPath(disk_path_hash);

    git_repository* repo = nullptr;
    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags = GIT_REPOSITORY_INIT_BARE | GIT_REPOSITORY_INIT_MKDIR;

    int error = git_repository_init_ext(&repo, full_path.c_str(), &opts);

    if (error < 0)
    {
      const git_error* e = git_error_last();
      std::cerr << "[!] Libgit2 init failed: " << (e ? e->message : "Unknown error") << std::endl;
      return false;
    }

    git_repository_free(repo);
    return true;
  }

  bool GitStorage::DeleteRepo(const std::string &disk_path_hash)
  {
    std::string full_path = GetFullPath(disk_path_hash);

    try
    {
      if (fs::exists(full_path))
      {
        fs::remove_all(full_path);
        return true;
      }
    } catch (const std::exception& e)
    {
      std::cerr << "[!] Delete failed: " << e.what() << std::endl;
    }

    return false;
  }

  bool GitStorage::Exists(const std::string &disk_path_hash)
  {
    return fs::exists(GetFullPath(disk_path_hash));
  }
}
