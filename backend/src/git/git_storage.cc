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

  std::optional<std::string> GitStorage::GetFileContent(const std::string &disk_path_hash, const std::string &branch,
    const std::string &filepath)
  {
    std::string full_path = GetFullPath(disk_path_hash);
    git_repository* repo = nullptr;
    if (git_repository_open(&repo, full_path.c_str()) != 0) return std::nullopt;

    git_object* obj = nullptr;
    std::string ref = branch.empty() ? "HEAD" : branch;
    if (git_revparse_single(&obj, repo, ref.c_str()) != 0)
    {
      git_repository_free(repo);
      return std::nullopt;
    }

    git_tree* tree = nullptr;
    if (git_object_peel((git_object**)&tree, obj, GIT_OBJ_TREE) != 0)
    {
      git_object_free(obj);
      git_repository_free(repo);
      return std::nullopt;
    }
    git_object_free(obj);

    git_tree_entry* entry = nullptr;
    std::optional<std::string> content = std::nullopt;

    if (git_tree_entry_bypath(&entry, tree, filepath.c_str()) == 0)
    {
      git_object* blob_obj = nullptr;
      if (git_tree_entry_to_object(&blob_obj, repo, entry) == 0)
      {
        if (git_object_type(blob_obj) == GIT_OBJ_BLOB)
        {
          const char* raw = (const char*)git_blob_rawcontent((git_blob*)blob_obj);
          content = std::string(raw, git_blob_rawsize((git_blob*)blob_obj));
        }
        git_object_free(blob_obj);
      }
      git_tree_entry_free(entry);
    }

    git_tree_free(tree);
    git_repository_free(repo);
    return content;
  }

  bool GitStorage::CreateBranch(const std::string &disk_path_hash, const std::string &branch_name, const std::string &target_branch)
  {
    std::string full_path = GetFullPath(disk_path_hash);
    git_repository* repo = nullptr;
    if (git_repository_open(&repo, full_path.c_str()) != 0) return false;

    git_object* target_obj = nullptr;
    std::string ref = target_branch.empty() ? "HEAD" : target_branch;
    if (git_revparse_single(&target_obj, repo, ref.c_str()) != 0) {
      git_repository_free(repo);
      return false;
    }

    git_commit* target_commit = nullptr;
    if (git_commit_lookup(&target_commit, repo, git_object_id(target_obj)) != 0) {
      git_object_free(target_obj);
      git_repository_free(repo);
      return false;
    }

    git_reference* new_branch = nullptr;
    bool success = (git_branch_create(&new_branch, repo, branch_name.c_str(), target_commit, 0) == 0);

    if (new_branch) git_reference_free(new_branch);
    git_commit_free(target_commit);
    git_object_free(target_obj);
    git_repository_free(repo);
    return success;
  }

  bool GitStorage::DeleteBranch(const std::string &disk_path_hash, const std::string &branch_name)
  {
    std::string full_path = GetFullPath(disk_path_hash);
    git_repository* repo = nullptr;
    if (git_repository_open(&repo, full_path.c_str()) != 0) return false;

    git_reference* branch_ref = nullptr;
    if (git_branch_lookup(&branch_ref, repo, branch_name.c_str(), GIT_BRANCH_LOCAL) != 0) {
      git_repository_free(repo);
      return false;
    }

    bool success = (git_branch_delete(branch_ref) == 0);

    git_reference_free(branch_ref);
    git_repository_free(repo);
    return success;
  }

  bool GitStorage::MergeBranch(const std::string &disk_path_hash, const std::string &source_branch, const std::string &target_branch)
  {
      // Since it's a bare repository (created with GIT_REPOSITORY_INIT_BARE),
      // merge is complex. Let's do it using commit mechanism or fail.
      std::string full_path = GetFullPath(disk_path_hash);
      git_repository* repo = nullptr;
      if (git_repository_open(&repo, full_path.c_str()) != 0) return false;

      git_annotated_commit* their_head = nullptr;
      git_reference* source_ref = nullptr;
      if (git_reference_lookup(&source_ref, repo, ("refs/heads/" + source_branch).c_str()) != 0) {
          git_repository_free(repo);
          return false;
      }
      git_annotated_commit_from_ref(&their_head, repo, source_ref);

      git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
      git_index* index = nullptr;

      git_commit* target_commit = nullptr;
      git_reference* target_ref = nullptr;
      if (git_reference_lookup(&target_ref, repo, ("refs/heads/" + target_branch).c_str()) != 0) {
          git_reference_free(source_ref);
          git_annotated_commit_free(their_head);
          git_repository_free(repo);
          return false;
      }
      git_commit_lookup(&target_commit, repo, git_reference_target(target_ref));

      git_commit* source_commit = nullptr;
      git_commit_lookup(&source_commit, repo, git_reference_target(source_ref));

      if (git_merge_commits(&index, repo, target_commit, source_commit, &merge_opts) != 0) {
          // Merge failed (conflicts or error)
          git_commit_free(source_commit);
          git_commit_free(target_commit);
          git_reference_free(target_ref);
          git_reference_free(source_ref);
          git_annotated_commit_free(their_head);
          git_repository_free(repo);
          return false;
      }

      if (git_index_has_conflicts(index)) {
          git_index_free(index);
          git_commit_free(source_commit);
          git_commit_free(target_commit);
          git_reference_free(target_ref);
          git_reference_free(source_ref);
          git_annotated_commit_free(their_head);
          git_repository_free(repo);
          return false;
      }

      git_oid tree_oid;
      git_index_write_tree(&tree_oid, index);
      git_tree* merge_tree = nullptr;
      git_tree_lookup(&merge_tree, repo, &tree_oid);

      git_signature* me;
      git_signature_now(&me, "Codelab System", "system@codelab.local");

      git_oid new_commit_id;
      const git_commit* parents[2] = { target_commit, source_commit };
      bool merge_success = (git_commit_create(&new_commit_id, repo, target_ref ? git_reference_name(target_ref) : nullptr, me, me,
                             nullptr, ("Merge branch '" + source_branch + "' into " + target_branch).c_str(), merge_tree, 2, parents) == 0);

      git_signature_free(me);
      git_tree_free(merge_tree);
      git_index_free(index);
      git_commit_free(source_commit);
      git_commit_free(target_commit);
      git_reference_free(target_ref);
      git_reference_free(source_ref);
      git_annotated_commit_free(their_head);
      git_repository_free(repo);

      return merge_success;
  }
}
