#ifndef CODELAB_FILE_ENTRY_H
#define CODELAB_FILE_ENTRY_H

#include <string>

namespace codelab::models
{
  struct FileEntry
  {
    std::string name;
    std::string path;
    bool is_directory;
    std::string last_commit_msg;
    std::string last_commit_date;
    int size;
  };
}

#endif