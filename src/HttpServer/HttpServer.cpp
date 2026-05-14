#include "HttpServer.h"
#include "GameCommands.h"
#include "libs/httplib.h"

#include <thread>
#include <string>
#include <optional>

// ---------------------------------------------------------------------------
// Small helper: turn a CommandResult into an HTTP response.
// ---------------------------------------------------------------------------
namespace
{
    void ApplyResult(CommandResult const& result, httplib::Response& res,
                     std::string const& contentType = "application/json")
    {
        if (result.ok)
        {
            res.set_content(result.value, contentType);
        }
        else
        {
            res.status = result.httpStatus;
            res.set_content(
                "{\"error\":\"" + result.error + "\"}",
                "application/json");
        }
    }

    /// Safe stof / stoul that return nullopt instead of throwing.
    std::optional<float>    TryParseFloat(std::string const& s)
    {
        if (s.empty()) return std::nullopt;
        try { return std::stof(s); } catch (...) { return std::nullopt; }
    }

    std::optional<uint32_t> TryParseUint(std::string const& s)
    {
        if (s.empty()) return std::nullopt;
        try { return static_cast<uint32_t>(std::stoul(s)); } catch (...) { return std::nullopt; }
    }

    std::optional<int32_t>  TryParseInt(std::string const& s)
    {
        if (s.empty()) return std::nullopt;
        try { return std::stoi(s); } catch (...) { return std::nullopt; }
    }

    /// Parse a minimal JSON body for POST endpoints (no external dependency needed).
    /// Returns the value for a given key from a flat {"key":"value",...} body.
    std::string JsonBodyGet(std::string const& body, std::string const& key)
    {
        std::string needle = "\"" + key + "\"";
        auto pos = body.find(needle);
        if (pos == std::string::npos) return "";

        pos = body.find(':', pos + needle.size());
        if (pos == std::string::npos) return "";

        // Skip whitespace
        while (++pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) {}

        if (pos >= body.size()) return "";

        if (body[pos] == '"')
        {
            // String value
            std::string result;
            ++pos;
            while (pos < body.size() && body[pos] != '"')
            {
                if (body[pos] == '\\' && pos + 1 < body.size())
                {
                    ++pos; // skip escape
                    result += body[pos];
                }
                else
                {
                    result += body[pos];
                }
                ++pos;
            }
            return result;
        }
        else
        {
            // Numeric / boolean value: read until delimiter
            size_t end = pos;
            while (end < body.size() && body[end] != ',' && body[end] != '}' && !std::isspace(static_cast<unsigned char>(body[end])))
                ++end;
            return body.substr(pos, end - pos);
        }
    }
} // anonymous namespace

// ---------------------------------------------------------------------------

void HttpServer::Start()
{
    std::thread([]()
    {
        httplib::Server svr;

        // ------------------------------------------------------------------
        // GET /server
        // ------------------------------------------------------------------
        svr.Get("/server", [](const httplib::Request&, httplib::Response& res)
        {
            ApplyResult(GameCommands::GetServerInfo(), res);
        });

        // ------------------------------------------------------------------
        // GET /time
        // ------------------------------------------------------------------
        svr.Get("/time", [](const httplib::Request&, httplib::Response& res)
        {
            ApplyResult(GameCommands::GetTime(), res);
        });

        // ------------------------------------------------------------------
        // GET /player?name=…
        // ------------------------------------------------------------------
        svr.Get("/player", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(GameCommands::GetPlayer(req.get_param_value("name")), res);
        });

        // ------------------------------------------------------------------
        // GET /players[?map=…&x=…&y=…&range=…]
        // ------------------------------------------------------------------
        svr.Get("/players", [](const httplib::Request& req, httplib::Response& res)
        {
            ApplyResult(GameCommands::GetPlayers(
                TryParseUint(req.get_param_value("map")),
                TryParseFloat(req.get_param_value("x")),
                TryParseFloat(req.get_param_value("y")),
                TryParseFloat(req.get_param_value("range"))
            ), res);
        });

        // ------------------------------------------------------------------
        // GET /groundz?map=…&x=…&y=…
        // ------------------------------------------------------------------
        svr.Get("/groundz", [](const httplib::Request& req, httplib::Response& res)
        {
            auto mapId = TryParseUint(req.get_param_value("map"));
            auto x     = TryParseFloat(req.get_param_value("x"));
            auto y     = TryParseFloat(req.get_param_value("y"));

            if (!mapId || !x || !y)
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing or invalid parameters\"}", "application/json");
                return;
            }

            ApplyResult(GameCommands::GetGroundZ(*mapId, *x, *y), res);
        });

        // ------------------------------------------------------------------
        // GET /zone?map=…&x=…&y=…&z=…
        // ------------------------------------------------------------------
        svr.Get("/zone", [](const httplib::Request& req, httplib::Response& res)
        {
            auto mapId = TryParseUint(req.get_param_value("map"));
            auto x     = TryParseFloat(req.get_param_value("x"));
            auto y     = TryParseFloat(req.get_param_value("y"));
            auto z     = TryParseFloat(req.get_param_value("z"));

            if (!mapId || !x || !y || !z)
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing or invalid parameters\"}", "application/json");
                return;
            }

            ApplyResult(GameCommands::GetZone(*mapId, *x, *y, *z), res);
        });

        // ------------------------------------------------------------------
        // POST /broadcast
        // Body: { "message": "…", "type": "system"|"say"|"yell",
        //         "mapId": 0, "x": 0.0, "y": 0.0, "z": 0.0 }
        // ------------------------------------------------------------------
        svr.Post("/broadcast", [](const httplib::Request& req, httplib::Response& res)
        {
            BroadcastParams params;
            params.message = JsonBodyGet(req.body, "message");
            params.type    = JsonBodyGet(req.body, "type");
            if (params.type.empty()) params.type = "system";

            auto mapStr = JsonBodyGet(req.body, "mapId");
            auto xStr   = JsonBodyGet(req.body, "x");
            auto yStr   = JsonBodyGet(req.body, "y");
            auto zStr   = JsonBodyGet(req.body, "z");

            if (!mapStr.empty()) params.mapId = TryParseUint(mapStr);
            if (!xStr.empty())   params.x     = TryParseFloat(xStr);
            if (!yStr.empty())   params.y     = TryParseFloat(yStr);
            if (!zStr.empty())   params.z     = TryParseFloat(zStr);

            ApplyResult(GameCommands::Broadcast(params), res);
        });

        // ------------------------------------------------------------------
        // POST /spawn
        // Body: { "entry": 1234, "mapId": 0, "x": 0.0, "y": 0.0, "z": 0.0,
        //         "orientation": 0.0, "despawnSecs": 0, "name": "" }
        // ------------------------------------------------------------------
        svr.Post("/spawn", [](const httplib::Request& req, httplib::Response& res)
        {
            SpawnParams params;

            auto entry = TryParseUint(JsonBodyGet(req.body, "entry"));
            if (!entry)
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing or invalid 'entry'\"}", "application/json");
                return;
            }
            params.entry = *entry;

            if (auto v = TryParseUint(JsonBodyGet(req.body, "mapId")))       params.mapId      = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "x")))          params.x          = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "y")))          params.y          = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "z")))          params.z          = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "orientation"))) params.orientation = *v;
            if (auto v = TryParseUint(JsonBodyGet(req.body, "despawnSecs"))) params.despawnSecs = *v;
            params.name = JsonBodyGet(req.body, "name");

            ApplyResult(GameCommands::SpawnCreature(params), res);
        });

        // ------------------------------------------------------------------
        // DELETE /spawn/:guid
        // ------------------------------------------------------------------
        svr.Delete(R"(/spawn/(\d+))", [](const httplib::Request& req, httplib::Response& res)
        {
            uint64_t guid = 0;
            try { guid = std::stoull(req.matches[1]); }
            catch (...)
            {
                res.status = 400;
                res.set_content("{\"error\":\"invalid guid\"}", "application/json");
                return;
            }
            ApplyResult(GameCommands::DespawnCreature(guid), res);
        });

        // ------------------------------------------------------------------
        // POST /teleport
        // Body: { "name": "…", "mapId": 0, "x": 0.0, "y": 0.0,
        //         "z": 0.0, "orientation": 0.0 }
        // ------------------------------------------------------------------
        svr.Post("/teleport", [](const httplib::Request& req, httplib::Response& res)
        {
            TeleportParams params;
            params.playerName = JsonBodyGet(req.body, "name");
            if (params.playerName.empty())
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing 'name' field\"}", "application/json");
                return;
            }

            if (auto v = TryParseUint(JsonBodyGet(req.body, "mapId")))        params.mapId      = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "x")))           params.x          = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "y")))           params.y          = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "z")))           params.z          = *v;
            if (auto v = TryParseFloat(JsonBodyGet(req.body, "orientation")))  params.orientation = *v;

            ApplyResult(GameCommands::TeleportPlayer(params), res);
        });

        // ------------------------------------------------------------------
        // POST /item/give
        // Body: { "name": "…", "itemEntry": 12345, "count": 1 }
        // ------------------------------------------------------------------
        svr.Post("/item/give", [](const httplib::Request& req, httplib::Response& res)
        {
            ItemParams params;
            params.playerName = JsonBodyGet(req.body, "name");
            if (params.playerName.empty())
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing 'name' field\"}", "application/json");
                return;
            }

            auto entry = TryParseUint(JsonBodyGet(req.body, "itemEntry"));
            if (!entry)
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing or invalid 'itemEntry'\"}", "application/json");
                return;
            }
            params.itemEntry = *entry;

            if (auto v = TryParseInt(JsonBodyGet(req.body, "count"))) params.count = *v;

            ApplyResult(GameCommands::ModifyItem(params), res);
        });

        // ------------------------------------------------------------------
        // POST /vital
        // Body: { "name": "…", "vitalType": "health", "value": -1 }
        // ------------------------------------------------------------------
        svr.Post("/vital", [](const httplib::Request& req, httplib::Response& res)
        {
            SetVitalParams params;
            params.playerName = JsonBodyGet(req.body, "name");
            if (params.playerName.empty())
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing 'name' field\"}", "application/json");
                return;
            }

            auto vt = JsonBodyGet(req.body, "vitalType");
            if (!vt.empty()) params.vitalType = vt;

            if (auto v = TryParseInt(JsonBodyGet(req.body, "value"))) params.value = *v;

            ApplyResult(GameCommands::SetVital(params), res);
        });

        // ------------------------------------------------------------------
        // POST /kick
        // Body: { "name": "…", "reason": "…" }
        // ------------------------------------------------------------------
        svr.Post("/kick", [](const httplib::Request& req, httplib::Response& res)
        {
            std::string name   = JsonBodyGet(req.body, "name");
            std::string reason = JsonBodyGet(req.body, "reason");

            if (name.empty())
            {
                res.status = 400;
                res.set_content("{\"error\":\"missing 'name' field\"}", "application/json");
                return;
            }

            ApplyResult(GameCommands::KickPlayer(name, reason), res);
        });

        svr.listen("0.0.0.0", 8080);

    }).detach();
}