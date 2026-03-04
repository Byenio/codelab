#ifndef CODELAB_DIRECTORY_H
#define CODELAB_DIRECTORY_H

#include <string>
#include <optional>

namespace codelab::models
{
  struct Directory
  {
    int id = 0;
    int user_id;
    std::optional<int> parent_id;
    std::string name;
  };
}

#endif //CODELAB_DIRECTORY_H