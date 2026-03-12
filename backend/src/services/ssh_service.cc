#include "services/ssh_service.h"

#include <fstream>
#include <ios>
#include <iostream>

namespace codelab::services
{
  std::optional<int> SSHService::AddKey(int user_id, const std::string &title, const std::string &key_content)
  {
    if (key_content.find("ssh-") != 0 && key_content.find("ecdsa-") != 0)
    {
      return std::nullopt;
    }

    auto id = ssh_dao_.Create(user_id, title, key_content);

    if (id)
    {
      SyncAuthorizedKeysFile();
    }

    return id;
  }

  bool SSHService::RemoveKey(int id, int user_id)
  {
    if (ssh_dao_.Delete(id, user_id))
    {
      SyncAuthorizedKeysFile();
      return true;
    }

    return false;
  }

  bool SSHService::SyncAuthorizedKeysFile()
  {
    std::string auth_file_path = "/home/git/.ssh/authorized_keys";
    std::string shell_path = "/app/codelab_shell";

    std::ofstream file(auth_file_path, std::ios::trunc);
    if (!file.is_open())
    {
      std::cerr << "[!] Failed to open authorized keys at: " << auth_file_path << std::endl;
      return false;
    }

    auto all_keys = ssh_dao_.GetAllKeys();
    for (const auto& key : all_keys)
    {
      file << "command=\"" << shell_path << " " << key.user_id << "\",";
      file << "no-port-forwarding,no-X11-forwarding,no-agent-forwarding,no-pty ";
      file << key.key_content << "\n";
    }

    file.close();
    return true;
  }
}
