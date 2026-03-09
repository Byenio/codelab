#include "services/auth_service.h"
#include "core/config.h"
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <chrono>
#include <sodium.h>
#include <crow/utility.h>

namespace codelab::services
{
  using jwt_traits = jwt::traits::nlohmann_json;

  std::optional<std::string> AuthService::Login(const std::string &username, const std::string &password)
  {
    auto user = user_dao_.FindByUsername(username);
    if (!user) return std::nullopt;

    if (crypto_pwhash_str_verify(user->password_hash.c_str(), password.c_str(), password.length()) != 0)
    {
      return std::nullopt;
    }

    std::string secret = core::Config::GetInstance().GetString("JWT_SECRET");

    auto token = jwt::create<jwt_traits>()
      .set_issuer("codelab")
      .set_type("JWS")
      .set_payload_claim("user_id", jwt::basic_claim<jwt_traits>(std::to_string(user->id)))
      .set_issued_at(std::chrono::system_clock::now())
      .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
      .sign(jwt::algorithm::hs256{secret});

    return token;
  }

  std::optional<int> AuthService::VerifyToken(const std::string &token)
  {
    try
    {
      std::string secret = core::Config::GetInstance().GetString("JWT_SECRET");

      auto decoded = jwt::decode<jwt_traits>(token);
      auto verifier = jwt::verify<jwt_traits>()
        .allow_algorithm(jwt::algorithm::hs256{secret})
        .with_issuer("codelab");

      verifier.verify(decoded);

      auto claim = decoded.get_payload_claim("user_id");
      return std::stoi(claim.as_string());
    } catch (const std::exception&)
    {
      return std::nullopt;
    }
  }

  std::optional<int> AuthService::VerifyBasicAuth(const std::string &auth_header)
  {
    if (auth_header.substr(0, 6) != "Basic ") return std::nullopt;

    std::string encoded = auth_header.substr(6);
    std::string decoded = crow::utility::base64decode(encoded, encoded.size());

    auto colon_pos = decoded.find(':');
    if (colon_pos == std::string::npos) return std::nullopt;

    std::string username = decoded.substr(0, colon_pos);
    std::string password = decoded.substr(colon_pos + 1);

    auto user = user_dao_.FindByUsername(username);
    if (!user) return std::nullopt;

    if (crypto_pwhash_str_verify(user->password_hash.c_str(), password.c_str(), password.length()) != 0)
    {
      return std::nullopt;
    }

    return user->id;
  }
}
