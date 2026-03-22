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
    std::optional<models::Repository> FindById(int id);

    std::vector<models::Repository> ListByDirectory(int user_id, std::optional<int> directory_id);
    std::vector<models::Repository> ListByUser(int user_id);
  };
}

#endif //CODELAB_REPOSITORY_DAO_H