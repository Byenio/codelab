#include "git/git_storage.h"
#include <iostream>
#include <filesystem>
#include <git2.h>

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

  std::vector<models::FileEntry> GitStorage::ListFiles(const std::string &disk_path_hash, const std::string &branch, const std::string &directory_path)
  {
    std::vector<models::FileEntry> entries;
    std::string full_path = GetFullPath(disk_path_hash);

    git_repository* repo = nullptr;
    if (git_repository_open(&repo, full_path.c_str()) != 0) return entries;

    git_object* obj = nullptr;
    std::string ref = branch.empty() ? "HEAD" : branch;

    if (git_revparse_single(&obj, repo, ref.c_str()) != 0) return entries;

    git_tree* tree = nullptr;
    if (git_object_peel((git_object**)&tree, obj, GIT_OBJ_TREE) != 0)
    {
      git_object_free(obj);
      git_repository_free(repo);
      return entries;
    }
    git_object_free(obj);

    git_tree* subtree = nullptr;
    bool free_subtree = false;

    if (!directory_path.empty() && directory_path != "/")
    {
      git_tree_entry* entry = nullptr;
      if (git_tree_entry_bypath(&entry, tree, directory_path.c_str()) == 0)
      {
        const git_oid* oid = git_tree_entry_id(entry);
        if (git_tree_lookup(&subtree, repo, oid) != 0)
        {
          git_tree_entry_free(entry);
          git_tree_free(tree);
          git_repository_free(repo);
          return entries;
        }
        free_subtree = true;
        git_tree_entry_free(entry);
      }
      else
      {
        git_tree_free(tree);
        git_repository_free(repo);
        return entries;
      }
    }
    else
    {
      subtree = tree;
    }

    size_t count = git_tree_entrycount(subtree);
    for (size_t i = 0; i < count; ++i)
    {
      const git_tree_entry* entry = git_tree_entry_byindex(subtree, i);

      models::FileEntry file;
      file.name = git_tree_entry_name(entry);
      file.path = (directory_path == "/" || directory_path.empty())
                  ? file.name
                  : directory_path + "/" + file.name;

      git_filemode_t mode = git_tree_entry_filemode(entry);
      file.is_directory = (mode == GIT_FILEMODE_TREE);
      file.size = 0;

      if (!file.is_directory) {
        git_object* blob_obj = nullptr;
        if (git_tree_entry_to_object(&blob_obj, repo, entry) == 0) {
          file.size = git_blob_rawsize((git_blob*)blob_obj);
          git_object_free(blob_obj);
        }
      }

      entries.push_back(file);
    }

    if (free_subtree && subtree) git_tree_free(subtree);
    git_tree_free(tree);
    git_repository_free(repo);

    return entries;
  }
}
