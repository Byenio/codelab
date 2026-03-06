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
}
