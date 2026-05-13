#include "HttpServer.h"

#include "libs/httplib.h"

#include "MapMgr.h"
#include "Player.h"
#include "World.h"

#include <thread>

void HttpServer::Start()
{
    std::thread([]()
    {
        httplib::Server svr;

        svr.Get("/groundz", [](const httplib::Request& req, httplib::Response& res)
        {
            int mapId = std::stoi(req.get_param_value("map"));
            float x = std::stof(req.get_param_value("x"));
            float y = std::stof(req.get_param_value("y"));

            Map* map = sMapMgr->CreateBaseMap(mapId);

            if (!map)
            {
                res.status = 404;
                res.set_content("invalid map", "text/plain");
                return;
            }

            float z = map->GetHeight(
                x,
                y,
                MAX_HEIGHT
            );

            res.set_content(
                std::to_string(z),
                "text/plain"
            );
        });

        svr.listen("0.0.0.0", 8080);

    }).detach();
}