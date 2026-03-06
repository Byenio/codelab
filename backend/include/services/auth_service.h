#ifndef CODELAB_AUTH_SERVICE_H
#define CODELAB_AUTH_SERVICE_H

#include <optional>
#include <string>

#include "dao/user_dao.h"

namespace codelab::services
{
  class AuthService
  {
  public:
    std::optional<std::string> Login(const std::string& username, const std::string& password);
    std::optional<int> VerifyToken(const std::string& token);

  private:
    dao::UserDAO user_dao_;
  };
}

#endif //CODELAB_AUTH_SERVICE_H