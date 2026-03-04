#ifndef CODELAB_RESPOSITORY_H
#define CODELAB_RESPOSITORY_H

#include <string>
#include <optional>

namespace codelab::models
{
  struct Repository
  {
    int id = 0;
    int user_id;
    std::optional<int> directory_id;
    std::string name;
    std::string description;
    bool is_private;
    std::string disk_path_hash;
  };
}

#endif //CODELAB_RESPOSITORY_H