#include "services/git_http_service.h"

#include <iostream>
#include <sstream>
#include <reproc++/run.hpp>

namespace codelab::services
{
  std::optional<std::string> GitHttpService::GetInfoRefs(const std::string &repo_path, const std::string &service)
  {
    std::vector<std::string> args = {"git", service, "--stateless-rpc", "--advertise-refs", repo_path};
    auto output = ExecuteGitCommand(args);

    if (!output) return std::nullopt;

    std::string packet = "# service=" + service + "\n";
    std::stringstream ss;
    ss << std::hex << (packet.size() + 4);
    std::string len_str = ss.str();

    while (len_str.size() < 4)
    {
      len_str = "0" + len_str;
    }

    std::string final_response = len_str + packet + "0000" + *output;
    return final_response;
  }

  std::optional<std::string> GitHttpService::HandleRpc(const std::string &repo_path, const std::string &service, const std::string &input_data)
  {
    std::vector<std::string> args = {"git", service, "--stateless-rpc", repo_path};
    return ExecuteGitCommand(args, input_data);
  }

  std::optional<std::string> GitHttpService::ExecuteGitCommand(const std::vector<std::string> &args, const std::string &input)
  {
    reproc::process process;
    reproc::options options;

    options.redirect.parent = false;

    std::error_code ec = process.start(args, options);
    if (ec)
    {
      std::cerr << "[!] Git process start error: " << ec.message() << std::endl;
      return std::nullopt;
    }

    if (!input.empty())
    {
      const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
      process.write(data, input.size());
    }

    process.close(reproc::stream::in);

    std::string output;
    reproc::sink::string sink_out(output);

    ec = reproc::drain(process, sink_out, reproc::sink::null);
    if (ec)
    {
      std::cerr << "[!] Git drain error: " << ec.message() << std::endl;
      return std::nullopt;
    }

    auto [status, ec_wait] = process.wait(reproc::infinite);

    if (ec_wait || status != 0)
    {
      std::cerr << "[!] Git process failed with status: " << status << std::endl;
    }

    return output;
  }
}
