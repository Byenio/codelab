#include <gtest/gtest.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <memory>
#include <filesystem>
#include "test_database.h"
#include "api/routes.h"
#include "core/config.h"
#include "services/auth_service.h"
#include "services/repo_service.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

class GitHttpIntegrationTest : public DBTest {
protected:
    static std::unique_ptr<crow::App<codelab::middleware::AuthMiddleware>> app_ptr;
    static std::unique_ptr<std::thread> server_thread;

    const std::string base_url = "http://127.0.0.1:8080";

    static void SetUpTestSuite() {
        auto& config = codelab::core::Config::GetInstance();
        #ifdef _WIN32
                _putenv("REPO_STORAGE_PATH=test_repos/");
        #else
                setenv("REPO_STORAGE_PATH", "test_repos/", 1);
        #endif

        app_ptr = std::make_unique<crow::App<codelab::middleware::AuthMiddleware>>();
        app_ptr->loglevel(crow::LogLevel::Warning);
        codelab::api::RegisterRoutes(*app_ptr);

        server_thread = std::make_unique<std::thread>([](){
            app_ptr->port(8080).run();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    static void TearDownTestSuite() {
        if (app_ptr) app_ptr->stop();
        if (server_thread && server_thread->joinable()) server_thread->join();
        app_ptr.reset();
        server_thread.reset();
    }

    void SetUp() override {
        DBTest::SetUp();
        fs::create_directory("test_repos");
    }

    void TearDown() override {
        DBTest::TearDown();
        fs::remove_all("test_repos");
    }
};

std::unique_ptr<crow::App<codelab::middleware::AuthMiddleware>> GitHttpIntegrationTest::app_ptr = nullptr;
std::unique_ptr<std::thread> GitHttpIntegrationTest::server_thread = nullptr;

TEST_F(GitHttpIntegrationTest, SmartHttpHandshake) {
    // 1. Setup User & Repo via DAO directly (faster than API)
    codelab::dao::UserDAO uDao;
    auto user = uDao.Create("gituser", "gitpass", "git@test.com");
    ASSERT_TRUE(user.has_value());

    codelab::services::RepoService repoService("test_repos");
    auto repo = repoService.CreateRepository(user->id, std::nullopt, "httprepo", "", true, true); // Private, Readme
    ASSERT_TRUE(repo.has_value());

    std::string repo_url = base_url + "/git/" + std::to_string(repo->id) + ".git";

    // 2. Test Unauthenticated Access (Should Fail for Private Repo)
    cpr::Response r_unauth = cpr::Get(
        cpr::Url{repo_url + "/info/refs"},
        cpr::Parameters{{"service", "git-upload-pack"}}
    );
    EXPECT_EQ(r_unauth.status_code, 401);
    EXPECT_EQ(r_unauth.header["WWW-Authenticate"], "Basic realm=\"Codelab git\"");

    // 3. Test Authenticated Access (Should Succeed)
    cpr::Response r_auth = cpr::Get(
        cpr::Url{repo_url + "/info/refs"},
        cpr::Parameters{{"service", "git-upload-pack"}},
        cpr::Authentication{"gituser", "gitpass", cpr::AuthMode::BASIC}
    );

    ASSERT_EQ(r_auth.status_code, 200);
    EXPECT_EQ(r_auth.header["Content-Type"], "application/x-git-upload-pack-advertisement");

    // Validate Git Protocol Packet format
    // Expect: "001e# service=git-upload-pack\n0000" ...
    EXPECT_TRUE(r_auth.text.find("# service=git-upload-pack") != std::string::npos);
    EXPECT_TRUE(r_auth.text.find("refs/heads/master") != std::string::npos); // Should see the branch
}

TEST_F(GitHttpIntegrationTest, PublicRepoCloneWithoutAuth) {
    codelab::dao::UserDAO uDao;
    auto user = uDao.Create("gituser2", "gitpass2", "git2@test.com");

    codelab::services::RepoService repoService("test_repos");
    // is_private = false
    auto repo = repoService.CreateRepository(user->id, std::nullopt, "publicrepo", "", false, true);

    std::string repo_url = base_url + "/git/" + std::to_string(repo->id) + ".git";

    // Should succeed WITHOUT authentication
    cpr::Response r = cpr::Get(
        cpr::Url{repo_url + "/info/refs"},
        cpr::Parameters{{"service", "git-upload-pack"}}
    );

    ASSERT_EQ(r.status_code, 200);
    EXPECT_TRUE(r.text.find("refs/heads/master") != std::string::npos);
}