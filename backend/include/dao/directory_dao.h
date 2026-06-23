#ifndef CODELAB_DIRECTORY_DAO_H
#define CODELAB_DIRECTORY_DAO_H

#include <vector>
#include "models/directory.h"

namespace codelab::dao
{
  class DirectoryDAO
  {
  public:
    std::optional<int> Create(int user_id, std::optional<int> parent_id, const std::string& name);

    std::vector<models::Directory> ListByParent(int user_id, std::optional<int> parent_id);
    std::optional<models::Directory> FindById(int id);
    std::optional<models::Directory> FindByName(int user_id, std::optional<int> parent_id, const std::string& name);

    bool Delete(int id);
    bool UpdateParent(int id, std::optional<int> parent_id);
    bool UpdateName(int id, const std::string& name);
  };
}

#endif //CODELAB_DIRECTORY_DAO_H