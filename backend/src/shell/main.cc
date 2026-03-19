#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

#include "core/config.h"
#include "core/db.h"
#include "dao/repository_dao.h"
#include "dao/user_dao.h"
#include "models/user.h"

#ifdef _WIN32
int main() {
  std::cerr << "Codelab Shell is designed for Linux/Docker environments only." << std::endl;
  return 1;
}
#else
#include <unistd.h>

std::vector<std::string> Split(const std::string& str, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);

  while (std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }

  return tokens;
}

int main(int argc, char* argv[])
{
  std::streambuf* original_cout = std::cout.rdbuf(std::cerr.rdbuf());

  try {
    auto& db = codelab::core::Database::GetInstance();

    const char* env_path = std::getenv("DB_PATH");
    std::string db_path = env_path ? env_path : "/app/data/db/codelab.db";

    db.Connect(db_path);

  } catch (const std::exception& e) {
    std::cerr << "Shell Setup Error: " << e.what() << std::endl;
    return 1;
  }

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

  if (!repo_path_raw.empty() && repo_path_raw[0] == '/') {
    repo_path_raw.erase(0, 1);
  }

  if (repo_path_raw.size() > 4 && repo_path_raw.substr(repo_path_raw.size() - 4) == ".git") {
    repo_path_raw.resize(repo_path_raw.size() - 4);
  }

  size_t slash_pos = repo_path_raw.find('/');
  if (slash_pos == std::string::npos) {
    std::cerr << "Invalid repository path. Expected: username/repo" << std::endl;
    return 1;
  }

  std::string owner_name = repo_path_raw.substr(0, slash_pos);
  std::string repo_name = repo_path_raw.substr(slash_pos + 1);

  codelab::dao::UserDAO user_dao;
  std::optional<codelab::models::User> owner = user_dao.FindByUsername(owner_name);

  if (!owner) {
    std::cerr << "User not found: " << owner_name << std::endl;
    return 1;
  }

  int owner_id = owner->id;

  codelab::dao::RepositoryDAO repo_dao;
  auto repo = repo_dao.FindByName(owner_id, std::nullopt, repo_name);

  if (!repo) {
    std::cerr << "Repository not found" << std::endl;
    return 1;
  }

  // TODO: allow collaborators
  bool authorized = false;

  if (repo->user_id == user_id) {
    authorized = true;
  }

  else if (!repo->is_private && git_cmd == "git-upload-pack") {
    authorized = true;
  }

  if (!authorized) {
    std::cerr << "Access denied" << std::endl;
    return 1;
  }

  std::string storage_path = codelab::core::Config::GetInstance().GetString("REPO_STORAGE_PATH", "/app/data/repositories/");

  if (storage_path.back() != '/') storage_path += "/";

  std::string full_path = storage_path + repo->disk_path_hash + ".git";

  std::vector<char*> args;
  args.push_back(const_cast<char*>(git_cmd.c_str()));
  args.push_back(const_cast<char*>(full_path.c_str()));
  args.push_back(nullptr);

  execvp(args[0], args.data());

  perror("execvp failed");
  return 1;
}
#endif