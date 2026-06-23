#ifndef CODELAB_PULL_REQUEST_DAO_H
#define CODELAB_PULL_REQUEST_DAO_H

#include "models/pull_request.h"
#include <vector>
#include <optional>

namespace codelab::dao
{
  class PullRequestDAO
  {
  public:
    std::optional<int> Create(const models::PullRequest& pr);
    std::optional<models::PullRequest> FindById(int id);
    std::vector<models::PullRequest> ListByRepository(int repository_id);
    bool UpdateStatus(int id, const std::string& status);
  };
}

#endif //CODELAB_PULL_REQUEST_DAO_H
