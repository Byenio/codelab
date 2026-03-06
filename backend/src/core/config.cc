#include "core/config.h"
#include <fstream>
#include <cstdlib>

namespace codelab::core
{
  Config &Config::GetInstance()
  {
    static Config instance;
    return instance;
  }

  void Config::Load(const std::string &file_path)
  {
    std::ifstream file(file_path);
    std::string line;

    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#') continue;

      auto delimiter_pos = line.find('=');
      if (delimiter_pos != std::string::npos)
      {
        std::string key = line.substr(0, delimiter_pos);
        std::string value = line.substr(delimiter_pos + 1);
        vars_[key] = value;
      }
    }
  }

  std::string Config::GetString(const std::string &key, const std::string &default_val) const
  {
    auto it = vars_.find(key);
    if (it != vars_.end()) return it->second;

    if (const char* env_p = std::getenv(key.c_str()))
    {
      return std::string(env_p);
    }

    return default_val;
  }

  int Config::GetInt(const std::string &key, int default_val) const
  {
    std::string val = GetString(key);
    if (val.empty()) return default_val;

    try
    {
      return std::stoi(val);
    } catch (...)
    {
      return default_val;
    }
  }
}
