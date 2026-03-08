#include "git/git_viewer.h"
#include <iostream>
#include <ctime>

namespace codelab::git
{
  GitViewer::GitViewer(std::string repo_path) : path_(std::move(repo_path)) {};

  GitViewer::~GitViewer()
  {
    if (repo_) git_repository_free(repo_);
  }

  bool GitViewer::Open()
  {
    if (repo_) return true;

    int error = git_repository_open(&repo_, path_.c_str());
    if (error < 0)
    {
      std::cerr << "[!] Failed to open git repo at " << path_ << std::endl;
      return false;
    }

    return true;
  }

  std::vector<models::Branch> GitViewer::GetBranches()
  {
    std::vector<models::Branch> branches;
    if (!Open()) return branches;

    git_branch_iterator* it;
    if (git_branch_iterator_new(&it, repo_, GIT_BRANCH_LOCAL) != 0) return branches;

    git_reference* ref;
    git_branch_t type;

    while (git_branch_next(&ref, &type, it) == 0)
    {
      const char* branch_name;
      if (git_branch_name(&branch_name, ref) == 0)
      {
        models::Branch b;
        b.name = branch_name;
        b.is_head = git_branch_is_head(ref) == 1;

        const git_oid* oid = git_reference_target(ref);
        if (oid)
        {
          char hash_hex[41];
          git_oid_tostr(hash_hex, 41, oid);
          b.latest_commit_hash = hash_hex;
        }

        branches.push_back(b);
      }

      git_reference_free(ref);
    }

    git_branch_iterator_free(it);
    return branches;
  }

  std::vector<models::Commit> GitViewer::GetCommits(const std::string &branch_name, int limit)
  {
    std::vector<models::Commit> commits;
    if (!Open()) return commits;

    git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo_, branch_name.c_str()) != 0)
    {
      return commits;
    }

    git_revwalk* walker;
    git_revwalk_new(&walker, repo_);
    git_revwalk_sorting(walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
    git_revwalk_push(walker, git_object_id(obj));
    git_object_free(obj);

    git_oid oid;
    int count = 0;
    while (count < limit && git_revwalk_next(&oid, walker) == 0)
    {
      git_commit* commit;
      if (git_commit_lookup(&commit, repo_, &oid) == 0)
      {
        models::Commit c;

        char hash_hex[41];
        git_oid_tostr(hash_hex, sizeof(hash_hex), &oid);
        c.hash = hash_hex;

        c.message = git_commit_message(commit);

        const git_signature* sig = git_commit_author(commit);
        if (sig)
        {
          c.author_name = sig->name;
          c.author_email = sig->email;
          c.timestamp = sig->when.time;
        }

        if (git_commit_parentcount(commit) > 0)
        {
          const git_oid* parent_id = git_commit_parent_id(commit, 0);
          char p_hex[41];
          git_oid_tostr(p_hex, sizeof(p_hex), parent_id);
          c.parent_hash = p_hex;
        }

        commits.push_back(c);
        git_commit_free(commit);
        count++;
      }
    }

    git_revwalk_free(walker);
    return commits;
  }

  std::vector<TreeEntry> GitViewer::GetTree(const std::string &ref, const std::string &path)
  {
    std::vector<TreeEntry> entries;
    if (!Open()) return entries;

    git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo_, ref.c_str()) != 0) return entries;

    git_commit* commit = nullptr;
    if (git_object_peel(reinterpret_cast<git_object **>(&commit), obj, GIT_OBJECT_COMMIT) != 0)
    {
      git_object_free(obj);
      return entries;
    }

    git_tree* root_tree = nullptr;
    git_commit_tree(&root_tree, commit);

    git_tree* target_tree = nullptr;

    if (path.empty() || path == "/")
    {
      target_tree = root_tree;
    } else
    {
      git_tree_entry* entry = nullptr;
      if (git_tree_entry_bypath(&entry, root_tree, path.c_str()) == 0)
      {
        if (git_tree_entry_type(entry) == GIT_OBJECT_TREE)
        {
          git_tree_lookup(&target_tree, repo_, git_tree_entry_id(entry));
        }
        git_tree_entry_free(entry);
      }
    }

    if (target_tree)
    {
      size_t count = git_tree_entrycount(target_tree);
      for (size_t i = 0; i < count; i++)
      {
        const git_tree_entry* entry = git_tree_entry_byindex(target_tree, i);
        TreeEntry te;
        te.name = git_tree_entry_name(entry);
        te.type = (git_tree_entry_type(entry) == GIT_OBJECT_TREE) ? "tree" : "blob";

        char oid_str[GIT_OID_HEXSZ + 1];
        git_oid_tostr(oid_str, sizeof(oid_str), git_tree_entry_id(entry));
        te.oid = oid_str;

        entries.push_back(te);
      }
      if (target_tree != root_tree) git_tree_free(target_tree);
    }

    git_tree_free(root_tree);
    git_commit_free(commit);
    git_object_free(obj);

    return entries;
  }

  std::optional<std::string> GitViewer::GetBlob(const std::string &ref, const std::string &path)
  {
    if (!Open()) return std::nullopt;

    git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo_, ref.c_str()) != 0) return std::nullopt;

    git_commit* commit = nullptr;
    if (git_object_peel(reinterpret_cast<git_object **>(&commit), obj, GIT_OBJECT_COMMIT) != 0)
    {
      git_object_free(obj);
      return std::nullopt;
    }

    git_tree* tree = nullptr;
    git_commit_tree(&tree, commit);

    git_tree_entry* entry = nullptr;
    std::optional<std::string> content = std::nullopt;

    if (git_tree_entry_bypath(&entry, tree, path.c_str()) == 0)
    {
      if (git_tree_entry_type(entry) == GIT_OBJECT_BLOB)
      {
        git_blob* blob = nullptr;
        if (git_blob_lookup(&blob, repo_, git_tree_entry_id(entry)) == 0)
        {
          const char* raw = static_cast<const char*>(git_blob_rawcontent(blob));
          size_t size = git_blob_rawsize(blob);
          content = std::string(raw, size);
          git_blob_free(blob);
        }
      }
      git_tree_entry_free(entry);
    }

    git_tree_free(tree);
    git_commit_free(commit);
    git_object_free(obj);

    return content;
  }
}
