#ifndef CODELAB_SSH_KEY_DAO_H
#define CODELAB_SSH_KEY_DAO_H

#include <optional>
#include <string>
#include <vector>

#include "core/db.h"

namespace codelab::dao
{
  struct SSHKey
  {
    int id;
    int user_id;
    std::string title;
    std::string key_content;
    std::string fingerprint;
    std::string created_at;
  };

  class SSHKeyDAO
  {
  public:
    SSHKeyDAO() : db_(core::Database::GetInstance()) {};

    std::optional<int> Create(int user_id, const std::string& title, const std::string& key_content);
    std::vector<SSHKey> ListByUser(int user_id);
    bool Delete(int id, int user_id);
    std::vector<SSHKey> GetAllKeys();

  private:
    core::Database& db_;
  };
}

#endif //CODELAB_SSH_KEY_DAO_H