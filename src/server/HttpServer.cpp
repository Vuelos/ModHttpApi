#include <thread>
#include <string>

#include "libs/httplib.h"
#include "libs/json.hpp"

#include "HttpServer.h"
#include "common/HttpParamTypes.h"
#include "Config.h"
#include "controllers/server/ServerController.h"
#include "controllers/player/PlayerController.h"
#include "controllers/chat/ChatController.h"
#include "controllers/item/ItemController.h"
#include "controllers/creature/CreatureController.h"
#include "controllers/gameobject/GameObjectController.h"
#include "controllers/npc/NpcController.h"
#include "controllers/world/WorldController.h"

using json = nlohmann::json;

namespace
{
    void ApplyResult(CommandResult const& result, httplib::Response& res,
                     std::string const& contentType = "application/json")
    {
        if (result.ok)
            res.set_content(result.value, contentType);
        else
        {
            res.status = result.httpStatus;
            res.set_content(json{{"error", result.error}}.dump(), "application/json");
        }
    }

    void SendError(httplib::Response& res, int status, std::string const& msg)
    {
        res.status = status;
        res.set_content(json{{"error", msg}}.dump(), "application/json");
    }
}

void HttpServer::Start()
{
    std::thread([]()
    {
        httplib::Server svr;

        // ---- World / Server ----
        svr.Get("/server", [](const httplib::Request&, httplib::Response& res)
        {
            ApplyResult(ServerController::GetServerInfo(), res);
        });

        svr.Get("/time", [](const httplib::Request&, httplib::Response& res)
        {
            ApplyResult(ServerController::GetTime(), res);
        });

        svr.Get("/groundz", [](const httplib::Request& req, httplib::Response& res)
        {
            auto mapId = TryParseUint(req.get_param_value("map"));
            auto x     = TryParseFloat(req.get_param_value("x"));
            auto y     = TryParseFloat(req.get_param_value("y"));

            if (!mapId || !x || !y)
            {
                SendError(res, 400, "missing or invalid parameters");
                return;
            }
            ApplyResult(ServerController::GetGroundZ(*mapId, *x, *y), res);
        });

        svr.Get("/zone", [](const httplib::Request& req, httplib::Response& res)
        {
            auto mapId = TryParseUint(req.get_param_value("map"));
            auto x     = TryParseFloat(req.get_param_value("x"));
            auto y     = TryParseFloat(req.get_param_value("y"));
            auto z     = TryParseFloat(req.get_param_value("z"));

            if (!mapId || !x || !y || !z)
            {
                SendError(res, 400, "missing or invalid parameters");
                return;
            }
            ApplyResult(ServerController::GetZone(*mapId, *x, *y, *z), res);
        });

        // ---- Players ----
        svr.Get("/player", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(PlayerController::GetPlayer(req.get_param_value("name")), res);
        });

        svr.Get("/player/stats", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(PlayerController::GetPlayerStats(req.get_param_value("name")), res);
        });

        svr.Get("/player/skills", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(PlayerController::GetPlayerSkills(req.get_param_value("name")), res);
        });

        svr.Get("/player/quests", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(PlayerController::GetPlayerQuests(req.get_param_value("name")), res);
        });

        svr.Get("/player/equipment", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(PlayerController::GetPlayerEquipment(req.get_param_value("name")), res);
        });

        svr.Get("/players", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(PlayerController::GetPlayers(
                TryParseUint(req.get_param_value("map")),
                TryParseFloat(req.get_param_value("x")),
                TryParseFloat(req.get_param_value("y")),
                TryParseFloat(req.get_param_value("range"))
            ), res);
        });

        // ---- Chat ----
        svr.Post("/broadcast", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            BroadcastParams params;
            params.message = (*body).value("message", "");
            params.type    = (*body).value("type", "system");
            if (body->contains("mapId")) params.mapId = (*body)["mapId"].get<uint32_t>();
            if (body->contains("x"))     params.x     = (*body)["x"].get<float>();
            if (body->contains("y"))     params.y     = (*body)["y"].get<float>();
            if (body->contains("z"))     params.z     = (*body)["z"].get<float>();

            ApplyResult(ChatController::Broadcast(params), res);
        });

        // ---- Players (management) ----
        svr.Post("/teleport", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            std::string name = (*body).value("name", "");
            if (name.empty())
            {
                SendError(res, 400, "missing 'name' field");
                return;
            }

            ApplyResult(PlayerController::TeleportPlayer(name,
                (*body).value("mapId", 0),
                (*body).value("x", 0.f),
                (*body).value("y", 0.f),
                (*body).value("z", 0.f),
                (*body).value("orientation", 0.f)
            ), res);
        });

        svr.Post("/item/give", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            ItemParams params;
            params.playerName = (*body).value("name", "");
            if (params.playerName.empty())
            {
                SendError(res, 400, "missing 'name' field");
                return;
            }
            if (!body->contains("itemEntry"))
            {
                SendError(res, 400, "missing or invalid 'itemEntry'");
                return;
            }
            params.itemEntry = (*body)["itemEntry"].get<uint32_t>();
            params.count     = (*body).value("count", 1);

            ApplyResult(ItemController::ModifyItem(params), res);
        });

        svr.Post("/vital", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            std::string name = (*body).value("name", "");
            if (name.empty())
            {
                SendError(res, 400, "missing 'name' field");
                return;
            }

            ApplyResult(PlayerController::SetVital(name,
                (*body).value("vitalType", "health"),
                (*body).value("value", -1)
            ), res);
        });

        svr.Post("/kick", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            std::string name = (*body).value("name", "");
            if (name.empty())
            {
                SendError(res, 400, "missing 'name' field");
                return;
            }

            ApplyResult(PlayerController::KickPlayer(name, (*body).value("reason", "")), res);
        });

        svr.Get("/player/inventory", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(PlayerController::GetPlayerInventory(req.get_param_value("name")), res);
        });

        svr.Post("/player/gm", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            std::string name = (*body).value("name", "");
            if (name.empty())
            {
                SendError(res, 400, "missing 'name' field");
                return;
            }

            ApplyResult(PlayerController::ToggleGM(name, (*body).value("gm", true)), res);
        });

        svr.Post("/player/cooldowns/reset", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            ApplyResult(PlayerController::ResetCooldowns((*body).value("name", "")), res);
        });

        svr.Post("/player/morph", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            std::string name = (*body).value("name", "");
            if (name.empty())
            {
                SendError(res, 400, "missing 'name' field");
                return;
            }
            if (!body->contains("displayId"))
            {
                SendError(res, 400, "missing 'displayId' field");
                return;
            }

            ApplyResult(PlayerController::MorphPlayer(name, (*body)["displayId"].get<uint32_t>()), res);
        });

        svr.Post("/player/quest/complete", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            std::string name = (*body).value("name", "");
            uint32_t questId = (*body).value("questId", 0);
            if (name.empty())
            {
                SendError(res, 400, "missing 'name' field");
                return;
            }

            ApplyResult(PlayerController::CompleteQuest(name, questId), res);
        });

        // ---- Creatures ----
        svr.Post("/spawn", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            SpawnParams params;
            if (!body->contains("entry"))
            {
                SendError(res, 400, "missing 'entry' field");
                return;
            }
            params.entry = (*body)["entry"].get<uint32_t>();

            if (body->contains("mapId"))       params.mapId       = (*body)["mapId"].get<uint32_t>();
            if (body->contains("x"))           params.x           = (*body)["x"].get<float>();
            if (body->contains("y"))           params.y           = (*body)["y"].get<float>();
            if (body->contains("z"))           params.z           = (*body)["z"].get<float>();
            if (body->contains("orientation"))  params.orientation = (*body)["orientation"].get<float>();
            if (body->contains("despawnSecs"))  params.despawnSecs = (*body)["despawnSecs"].get<uint32_t>();
            if (body->contains("name"))         params.name        = (*body)["name"].get<std::string>();

            ApplyResult(CreatureController::SpawnCreature(params), res);
        });

        svr.Delete(R"(/spawn/(\d+))", [](const httplib::Request& req, httplib::Response& res)
        {
            uint64_t guid = 0;
            try { guid = std::stoull(req.matches[1]); }
            catch (...)
            {
                SendError(res, 400, "invalid guid");
                return;
            }

            ApplyResult(CreatureController::DespawnCreature(guid), res);
        });

        // ---- GameObjects ----
        svr.Post("/gameobject/spawn", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            SpawnParams params;
            if (!body->contains("entry"))
            {
                SendError(res, 400, "missing 'entry' field");
                return;
            }
            params.entry = (*body)["entry"].get<uint32_t>();

            if (body->contains("mapId"))       params.mapId       = (*body)["mapId"].get<uint32_t>();
            if (body->contains("x"))           params.x           = (*body)["x"].get<float>();
            if (body->contains("y"))           params.y           = (*body)["y"].get<float>();
            if (body->contains("z"))           params.z           = (*body)["z"].get<float>();
            if (body->contains("orientation"))  params.orientation = (*body)["orientation"].get<float>();
            if (body->contains("despawnSecs"))  params.despawnSecs = (*body)["despawnSecs"].get<uint32_t>();
            if (body->contains("name"))         params.name        = (*body)["name"].get<std::string>();

            ApplyResult(GameObjectController::SpawnGameObject(params), res);
        });

        svr.Delete(R"(/gameobject/spawn/(\d+))", [](const httplib::Request& req, httplib::Response& res)
        {
            uint64_t guid = 0;
            try { guid = std::stoull(req.matches[1]); }
            catch (...)
            {
                SendError(res, 400, "invalid guid");
                return;
            }

            ApplyResult(GameObjectController::DespawnGameObject(guid), res);
        });

        // ---- NPC ----
        svr.Get(R"(/npc/(\d+))", [](const httplib::Request& req, httplib::Response& res)
        {
            uint32_t entry = 0;
            try { entry = static_cast<uint32_t>(std::stoul(req.matches[1])); }
            catch (...)
            {
                SendError(res, 400, "invalid entry");
                return;
            }

            ApplyResult(NpcController::GetNpcInfo(entry), res);
        });

        svr.Post("/npc/say", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            if (!body->contains("guid"))
            {
                SendError(res, 400, "missing 'guid' field");
                return;
            }
            if (!body->contains("text"))
            {
                SendError(res, 400, "missing 'text' field");
                return;
            }

            ApplyResult(NpcController::NpcSay((*body)["guid"].get<uint64_t>(), (*body)["text"].get<std::string>(), (*body).value("type", "say")), res);
        });

        // ---- World ----
        svr.Get("/maps", [](const httplib::Request&, httplib::Response& res)
        {
            ApplyResult(WorldController::GetMaps(), res);
        });

        svr.Get("/nearby", [](const httplib::Request& req, httplib::Response& res)
        {
            auto mapId = TryParseUint(req.get_param_value("map"));
            auto x     = TryParseFloat(req.get_param_value("x"));
            auto y     = TryParseFloat(req.get_param_value("y"));
            auto range = TryParseFloat(req.get_param_value("range"));

            if (!mapId || !x || !y)
            {
                SendError(res, 400, "missing or invalid parameters (map, x, y required)");
                return;
            }

            ApplyResult(WorldController::GetNearby(*mapId, *x, *y, range), res);
        });

        svr.Post("/weather", [](const httplib::Request& req, httplib::Response& res)
        {
            auto body = ParseJsonBody(req.body);
            if (!body)
            {
                SendError(res, 400, "invalid JSON body");
                return;
            }

            uint32_t zoneId = (*body).value("zoneId", 0);
            uint32_t weatherType = (*body).value("weatherType", 0);
            float intensity = (*body).value("intensity", 0.5f);

            ApplyResult(WorldController::SetWeather(zoneId, weatherType, intensity), res);
        });

        std::string bindAddr = sConfigMgr->GetOption<std::string>("HttpApi.BindAddress", "0.0.0.0");
        int port = sConfigMgr->GetOption<int32_t>("HttpApi.Port", 8080);
        svr.listen(bindAddr, port);
    }).detach();
}
