#include <iostream>
#include <process.h>
#include <string>
#include <vector>

#include "core/config.h"
#include "dao/repository_dao.h"

#ifdef _WIN32
int main() {
  std::cerr << "Codelab Shell is designed for Linux/Docker environments only." << std::endl;
  return 1;
}
#else

std::vector<std::string> Split(const std::string& std, char delimiter);

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Unauthorized - no user id provided" << std::endl;
    return 1;
  }
  int user_id = std::stoi(argv[1]);

  const char* original_command = std::getenv("SSH_ORIGINAL_COMMAND");

  if (!original_command)
  {
    std::cout << "Hi, welcome to Codelab. Interactive shell is disabled" << std::endl;
    return 0;
  }

  std::string cmd_str = original_command;
  std::vector<std::string> parts = Split(cmd_str, ' ');

  if (parts.empty()) return 1;

  std::string git_cmd = parts[0];
  std::string repo_path_raw = parts[1];

  if (repo_path_raw.size() >= 2 && repo_path_raw.front() == '\'' && repo_path_raw.back() == '\'') {
    repo_path_raw = repo_path_raw.substr(1, repo_path_raw.size() - 2);
  }

  int repo_id = 0;
  try
  {
    size_t dot_pos = repo_path_raw.find('.');
    if (dot_pos == std::string::npos) throw std::exception();
    repo_id = std::stoi(repo_path_raw.substr(0, dot_pos));
  } catch (...)
  {
    std::cerr << "Invalid repository path" << std::endl;
    return 1;
  }

  codelab::dao::RepositoryDAO repo_dao;
  auto repo = repo_dao.FindById(repo_id);

  if (!repo)
  {
    std::cerr << "Repository not found" << std::endl;
    return 1;
  }

  // TODO: allow collaborators
  bool authorized = false;
  if (git_cmd == "git-upload-pack")
  {
    if (!repo->is_private || repo->user_id == user_id) authorized = true;
  } else if (git_cmd == "git-receive-pack")
  {
    if (repo->user_id == user_id) authorized = true;
  }

  if (!authorized)
  {
    std::cerr << "Access denied" << std::endl;
    return 1;
  }

  std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "../../data/repositories/");
  std::string full_path = storage_path + repo->disk_path_hash + ".git";

  std::vector<std::string> arg_strings;
  arg_strings.push_back(git_cmd);
  arg_strings.push_back(full_path);

  std::vector<char*> args;
  for (auto& s : arg_strings) {
    args.push_back(const_cast<char*>(s.data()));
  }
  args.push_back(nullptr);

  execvp(args[0], args.data());

  perror("execvp failed");
  return 1;
}
#endif