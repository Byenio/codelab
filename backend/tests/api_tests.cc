#include <gtest/gtest.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <memory>

#include "test_database.h"
#include "api/routes.h" // We include your routes here!

using json = nlohmann::json;

class ApiIntegrationTest : public DBTest {
protected:
    // Pointers to hold our server and thread so we can control their lifecycle
    static std::unique_ptr<crow::App<codelab::middleware::AuthMiddleware>> app_ptr;
    static std::unique_ptr<std::thread> server_thread;

    const std::string base_url = "http://127.0.0.1:8080/api/v1";

    // Runs ONCE before any API tests start
    static void SetUpTestSuite() {
        app_ptr = std::make_unique<crow::App<codelab::middleware::AuthMiddleware>>();

        // Hide normal server logs during tests so it doesn't clutter the test output
        app_ptr->loglevel(crow::LogLevel::Warning);

        // Register the routes to this test instance of the app
        codelab::api::RegisterRoutes(*app_ptr);

        // Start the server in a background thread
        server_thread = std::make_unique<std::thread>([](){
            app_ptr->port(8080).run();
        });

        // Give the server a tiny bit of time to bind to the port
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Runs ONCE after all API tests finish
    static void TearDownTestSuite() {
        if (app_ptr) {
            app_ptr->stop(); // Signal Crow to shut down
        }
        if (server_thread && server_thread->joinable()) {
            server_thread->join(); // Wait for the thread to close cleanly
        }
        app_ptr.reset();
        server_thread.reset();
    }
};

// Initialize the static members
std::unique_ptr<crow::App<codelab::middleware::AuthMiddleware>> ApiIntegrationTest::app_ptr = nullptr;
std::unique_ptr<std::thread> ApiIntegrationTest::server_thread = nullptr;


// --- THE ACTUAL TEST ---

TEST_F(ApiIntegrationTest, FullAuthFlow) {
    // 1. REGISTER
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

    ASSERT_EQ(r_reg.status_code, 201) << "Failed to register user. Status: " << r_reg.status_code;

    // 2. LOGIN TO GET TOKEN
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
    ASSERT_TRUE(login_resp.contains("token"));
    std::string token = login_resp["token"];
    EXPECT_FALSE(token.empty());

    // 3. ACCESS PROTECTED ROUTE (Without Token - Should Fail)
    cpr::Response r_fail = cpr::Get(cpr::Url{base_url + "/directories"});
    EXPECT_EQ(r_fail.status_code, 401) << "Route is not protected!";

    // 4. ACCESS PROTECTED ROUTE (With Token - Should Succeed)
    cpr::Response r_success = cpr::Get(
        cpr::Url{base_url + "/directories"},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );

    EXPECT_EQ(r_success.status_code, 200);

    json dirs_resp = json::parse(r_success.text);
    EXPECT_TRUE(dirs_resp.contains("directories"));
}