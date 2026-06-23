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

    bool Delete(int id);
    bool UpdateDirectory(int id, std::optional<int> new_directory_id);

    // Collaborators
    bool AddCollaborator(int repo_id, int user_id);
    bool RemoveCollaborator(int repo_id, int user_id);
    bool IsCollaborator(int repo_id, int user_id);
    std::vector<int> ListCollaboratorIds(int repo_id);
  };
}

#endif //CODELAB_REPOSITORY_DAO_H