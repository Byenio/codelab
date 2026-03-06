#ifndef CODELAB_GIT_VIEWER_H
#define CODELAB_GIT_VIEWER_H

#include <string>
#include <vector>

#include "git_storage.h"
#include "models/git.h"

namespace codelab::git
{
  class GitViewer
  {
  public:
    explicit GitViewer(std::string repo_path);
    ~GitViewer();

    std::vector<models::Branch> GetBranches();
    std::vector<models::Commit> GetCommits(const std::string& branch_name = "HEAD", int limit = 10);

  private:
    std::string path_;
    git_repository* repo_ = nullptr;

    bool Open();
  };
}

#endif //CODELAB_GIT_VIEWER_H