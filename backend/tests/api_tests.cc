#include <gtest/gtest.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <memory>
#include "test_database.h"
#include "api/routes.h"

using json = nlohmann::json;

class ApiIntegrationTest : public DBTest {
protected:
    static std::unique_ptr<crow::App<codelab::middleware::AuthMiddleware>> app_ptr;
    static std::unique_ptr<std::thread> server_thread;

    const std::string base_url = "http://127.0.0.1:8080/api/v1";

    static void SetUpTestSuite() {
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
};

std::unique_ptr<crow::App<codelab::middleware::AuthMiddleware>> ApiIntegrationTest::app_ptr = nullptr;
std::unique_ptr<std::thread> ApiIntegrationTest::server_thread = nullptr;

TEST_F(ApiIntegrationTest, FullAuthFlow) {
    json reg_data = {
        {"username", "e2e_user"},
        {"password", "supersecret"},
        {"email", "e2e@test.com"}
    };
    cpr::Response r_reg = cpr::Post(
        cpr::Url{base_url + "/register"},
        cpr::Body{reg_data.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );
    ASSERT_EQ(r_reg.status_code, 201);

    json login_data = {
        {"username", "e2e_user"},
        {"password", "supersecret"}
    };
    cpr::Response r_login = cpr::Post(
        cpr::Url{base_url + "/login"},
        cpr::Body{login_data.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );
    ASSERT_EQ(r_login.status_code, 200);

    json login_resp = json::parse(r_login.text);
    std::string token = login_resp["token"];

    cpr::Response r_fail = cpr::Get(cpr::Url{base_url + "/directories"});
    EXPECT_EQ(r_fail.status_code, 401);

    cpr::Response r_success = cpr::Get(
        cpr::Url{base_url + "/directories"},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    EXPECT_EQ(r_success.status_code, 200);

    json repo_data = {
        {"name", "e2e_api_repo"},
        {"is_private", true},
        {"init_readme", true}
    };
    cpr::Response r_repo = cpr::Post(
        cpr::Url{base_url + "/repositories"},
        cpr::Body{repo_data.dump()},
        cpr::Header{{"Authorization", "Bearer " + token}, {"Content-Type", "application/json"}}
    );
    ASSERT_EQ(r_repo.status_code, 201);

    json repo_resp = json::parse(r_repo.text);
    std::string repo_id = std::to_string(repo_resp["id"].get<int>());

    cpr::Response r_branches = cpr::Get(
        cpr::Url{base_url + "/repositories/" + repo_id + "/branches"},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    ASSERT_EQ(r_branches.status_code, 200);
    json branches = json::parse(r_branches.text);
    ASSERT_EQ(branches.size(), 1);
    EXPECT_EQ(branches[0]["name"], "master");

    cpr::Response r_tree = cpr::Get(
        cpr::Url{base_url + "/repositories/" + repo_id + "/tree"},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    ASSERT_EQ(r_tree.status_code, 200);
    json tree = json::parse(r_tree.text);
    ASSERT_EQ(tree.size(), 1);
    EXPECT_EQ(tree[0]["name"], "README.md");

    cpr::Response r_blob = cpr::Get(
        cpr::Url{base_url + "/repositories/" + repo_id + "/blob?path=README.md"},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );
    ASSERT_EQ(r_blob.status_code, 200);
    json blob = json::parse(r_blob.text);
    std::string content = blob["content"];
    EXPECT_TRUE(content.find("# e2e_api_repo") != std::string::npos);
}