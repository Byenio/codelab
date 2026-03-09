#ifndef CODELAB_GIT_HTTP_SERVICE_H
#define CODELAB_GIT_HTTP_SERVICE_H

#include <optional>
#include <string>
#include <vector>

namespace codelab::services
{
  class GitHttpService
  {
  public:
    std::optional<std::string> GetInfoRefs(const std::string& repo_path, const std::string& service);
    std::optional<std::string> HandleRpc(const std::string& repo_path, const std::string& service, const std::string& input_data);

  private:
    std::optional<std::string> ExecuteGitCommand(const std::vector<std::string>& args, const std::string& input = "");
  };
}

#endif //CODELAB_GIT_HTTP_SERVICE_H