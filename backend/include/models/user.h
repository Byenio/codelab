#ifndef CODELAB_USER_H
#define CODELAB_USER_H

#include <string>

namespace codelab::models
{
  struct User
  {
    int id = 0;
    std::string username;
    std::string password_hash;
    std::string email;

    bool exists() const { return id != 0; }
  };
}

#endif //CODELAB_USER_H