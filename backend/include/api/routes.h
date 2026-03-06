#ifndef CODELAB_ROUTES_H
#define CODELAB_ROUTES_H

#include <crow.h>
#include "middleware/auth_middleware.h"

namespace codelab::api
{
  void RegisterRoutes(crow::App<middleware::AuthMiddleware>& app);
}

#endif //CODELAB_ROUTES_H