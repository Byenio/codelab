#ifndef CODELAB_GIT_H
#define CODELAB_GIT_H

#include <string>

namespace codelab::models
{
  struct Branch
  {
    std::string name;
    std::string latest_commit_hash;
    bool is_head;
  };

  struct Commit
  {
    std::string hash;
    std::string message;
    std::string author_name;
    std::string author_email;
    long long timestamp;
    std::string parent_hash;
  };
}

#endif //CODELAB_GIT_H