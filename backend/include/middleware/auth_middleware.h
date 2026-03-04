#ifndef CODELAB_AUTH_MIDDLEWARE_H
#define CODELAB_AUTH_MIDDLEWARE_H

#include <crow.h>
#include "models/user.h"
#include "dao/user_dao.h"

namespace codelab::middleware
{
  struct AuthMiddleware
  {
    struct context
    {
      models::User current_user;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx)
    {

    }

    void after_handle (crow::request& req, crow::response& res, context& ctx)
    {

    }
  };
}

#endif //CODELAB_AUTH_MIDDLEWARE_H