#ifndef CODELAB_USER_DAO_H
#define CODELAB_USER_DAO_H

#include "models/user.h"
#include <optional>
#include <string>

namespace codelab::dao
{
  class UserDAO
  {
  public:
    std::optional<models::User> Create(const std::string& username, const std::string& password, const std::string& email);
    std::optional<models::User> FindByUsername(const std::string& username);
    std::optional<models::User> FindById(int id);
  };
}

#endif //CODELAB_USER_DAO_H