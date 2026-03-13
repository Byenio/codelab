#include "git/git_storage.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace codelab::git
{
  bool GitStorage::InitRepo(const std::string &disk_path_hash, const std::string& repo_name, bool with_readme)
  {
    std::string full_path = GetFullPath(disk_path_hash);
    git_repository* repo = nullptr;

    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags = GIT_REPOSITORY_INIT_BARE | GIT_REPOSITORY_INIT_MKDIR;
    opts.initial_head = "master";

    if (git_repository_init_ext(&repo, full_path.c_str(), &opts) != 0)
    {
      const git_error* e = git_error_last();
      std::cerr << "[!] Libgit2 init failed: " << (e ? e->message : "Unknown error") << std::endl;
      return false;
    }

    if (with_readme)
    {
      std::string content = "# " + repo_name + "\n\nInitialized with Codelab.";
      git_oid blob_oid;
      git_blob_create_from_buffer(&blob_oid, repo, content.c_str(), content.size());

      git_treebuilder* builder = nullptr;
      git_treebuilder_new(&builder, repo, nullptr);
      git_treebuilder_insert(nullptr, builder, "README.md", &blob_oid, GIT_FILEMODE_BLOB);
      git_oid tree_oid;
      git_treebuilder_write(&tree_oid, builder);
      git_treebuilder_free(builder);

      git_tree* tree = nullptr;
      git_tree_lookup(&tree, repo, &tree_oid);

      git_signature* sig = nullptr;
      git_signature_now(&sig, "Codelab System", "system@codelab.local");

      git_oid commit_oid;
      git_commit_create(&commit_oid, repo, "HEAD", sig, sig, nullptr, "initial commit", tree, 0, nullptr);

      git_signature_free(sig);
      git_tree_free(tree);
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
