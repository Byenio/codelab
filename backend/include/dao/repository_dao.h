#ifndef CODELAB_REPOSITORY_DAO_H
#define CODELAB_REPOSITORY_DAO_H

#include "models/respository.h"
#include <vector>
#include <optional>

namespace codelab::dao
{
  class RepositoryDAO
  {
  public:
    std::optional<int> Create(const models::Repository& repo);

    std::optional<models::Repository> FindByName(int user_id, std::optional<int> directory_id, const std::string& name);
  };
}

#endif //CODELAB_REPOSITORY_DAO_H