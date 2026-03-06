#ifndef CODELAB_CONFIG_H
#define CODELAB_CONFIG_H

#include <string>
#include <unordered_map>

namespace codelab::core
{
  class Config
  {
  public:
    static Config& GetInstance();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void Load(const std::string& file_path);

    std::string GetString(const std::string& key, const std::string& default_val = "") const;
    int GetInt(const std::string& key, int default_val = 0) const;

  private:
    Config() = default;
    std::unordered_map<std::string, std::string> vars_;
  };
}

#endif //CODELAB_CONFIG_H