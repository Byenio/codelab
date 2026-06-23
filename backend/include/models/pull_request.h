#ifndef CODELAB_PULL_REQUEST_H
#define CODELAB_PULL_REQUEST_H

#include <string>

namespace codelab::models
{
  struct PullRequest
  {
    int id;
    int repository_id;
    int author_id;
    std::string title;
    std::string description;
    std::string source_branch;
    std::string target_branch;
    std::string status;
    std::string created_at;
  };
}

#endif //CODELAB_PULL_REQUEST_H
