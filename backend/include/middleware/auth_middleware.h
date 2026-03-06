#ifndef CODELAB_AUTH_MIDDLEWARE_H
#define CODELAB_AUTH_MIDDLEWARE_H

#include <crow.h>
#include "models/user.h"
#include "dao/user_dao.h"
#include "services/auth_service.h"

namespace codelab::middleware
{
  struct AuthMiddleware
  {
    struct context
    {
      int user_id = 0;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx)
    {
      std::string auth_header = req.get_header_value("Authorization");
      if (auth_header.empty()) return;

      if (auth_header.substr(0, 7) != "Bearer ") return;

      std::string token = auth_header.substr(7);

      services::AuthService auth;
      auto uid = auth.VerifyToken(token);

      if (uid)
      {
        ctx.user_id = *uid;
      }
    }

    void after_handle (crow::request& req, crow::response& res, context& ctx)
    {

    }
  };
}

#endif //CODELAB_AUTH_MIDDLEWARE_H