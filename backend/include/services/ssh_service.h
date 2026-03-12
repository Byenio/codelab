#ifndef CODELAB_SSH_SERVICE_H
#define CODELAB_SSH_SERVICE_H
#include <optional>
#include <string>

#include "dao/ssh_key_dao.h"

namespace codelab::services
{
  class SSHService
  {
  public:
    std::optional<int> AddKey(int user_id, const std::string& title, const std::string& key_content);
    bool RemoveKey(int id, int user_id);
    bool SyncAuthorizedKeysFile();

  private:
    dao::SSHKeyDAO ssh_dao_;
  };
}

#endif //CODELAB_SSH_SERVICE_H