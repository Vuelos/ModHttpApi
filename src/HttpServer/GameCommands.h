#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

/// Result returned by every GameCommand method.
/// ok == true  → use `value` (JSON string or plain text)
/// ok == false → use `error` and `httpStatus`
struct CommandResult
{
    bool        ok         = true;
    int         httpStatus = 200;
    std::string value;   ///< JSON payload on success
    std::string error;   ///< Human-readable error on failure

    static CommandResult Ok(std::string json)
    {
        CommandResult r;
        r.value = std::move(json);
        return r;
    }

    static CommandResult Fail(int status, std::string msg)
    {
        CommandResult r;
        r.ok         = false;
        r.httpStatus = status;
        r.error      = std::move(msg);
        return r;
    }
};

/// Parameters for NPC / creature spawning.
struct SpawnParams
{
    uint32_t    entry      = 0;
    uint32_t    mapId      = 0;
    float       x          = 0.f;
    float       y          = 0.f;
    float       z          = 0.f;
    float       orientation = 0.f;

    /// Optional: how long (seconds) before the spawn despawns (0 = permanent)
    uint32_t    despawnSecs = 0;

    /// Optional: custom display name (empty = use creature template name)
    std::string name;
};

/// Parameters for broadcasting a message to the global / world chat.
struct BroadcastParams
{
    std::string message;

    /// In-game chat type:
    ///   "say"    – SAY channel  (visible near sender coords)
    ///   "yell"   – YELL channel
    ///   "system" – SYSTEM (blue) notification (default)
    std::string type = "system";

    /// Only relevant for "say" / "yell": coordinates of the virtual sender
    std::optional<uint32_t> mapId;
    std::optional<float>    x, y, z;
};

/// Parameters for teleporting an online player.
struct TeleportParams
{
    std::string playerName;
    uint32_t    mapId = 0;
    float       x     = 0.f;
    float       y     = 0.f;
    float       z     = 0.f;
    float       orientation = 0.f;
};

/// Parameters for adding / removing an item in a player's inventory.
struct ItemParams
{
    std::string playerName;
    uint32_t    itemEntry = 0;
    int32_t     count     = 1; ///< negative → remove
};

/// Parameters for changing a player's health / power.
struct SetVitalParams
{
    std::string playerName;

    /// "health", "mana", "rage", "energy", "focus", "runic_power"
    std::string vitalType = "health";

    /// Absolute value to set; -1 means "set to maximum"
    int32_t value = -1;
};

// ---------------------------------------------------------------------------

class GameCommands
{
public:
    // -----------------------------------------------------------------------
    // World / server info
    // -----------------------------------------------------------------------

    /// GET /server   – uptime, player counts, session counts
    static CommandResult GetServerInfo();

    /// GET /time     – uptime, gameTime, startTime
    static CommandResult GetTime();

    // -----------------------------------------------------------------------
    // Player queries
    // -----------------------------------------------------------------------

    /// GET /player?name=…   – full player object
    static CommandResult GetPlayer(std::string const& name);

    /// GET /players[?map=…&x=…&y=…&range=…]  – array of player objects
    static CommandResult GetPlayers(
        std::optional<uint32_t> mapId,
        std::optional<float>    x,
        std::optional<float>    y,
        std::optional<float>    range);

    // -----------------------------------------------------------------------
    // World queries
    // -----------------------------------------------------------------------

    /// GET /groundz?map=…&x=…&y=…  – terrain height at (x,y)
    static CommandResult GetGroundZ(uint32_t mapId, float x, float y);

    /// GET /zone?map=…&x=…&y=…&z=… – zone/area ids and names
    static CommandResult GetZone(uint32_t mapId, float x, float y, float z);

    // -----------------------------------------------------------------------
    // Broadcast / chat
    // -----------------------------------------------------------------------

    /// POST /broadcast   – send a message to global / world chat
    static CommandResult Broadcast(BroadcastParams const& params);

    // -----------------------------------------------------------------------
    // Creature / NPC management
    // -----------------------------------------------------------------------

    /// POST /spawn        – spawn a creature in the world
    static CommandResult SpawnCreature(SpawnParams const& params);

    /// DELETE /spawn/:guid – despawn a previously spawned creature
    static CommandResult DespawnCreature(uint64_t guid);

    // -----------------------------------------------------------------------
    // Player management  (GM actions)
    // -----------------------------------------------------------------------

    /// POST /teleport     – teleport a player to given coords
    static CommandResult TeleportPlayer(TeleportParams const& params);

    /// POST /item/give    – give (or remove) an item from a player
    static CommandResult ModifyItem(ItemParams const& params);

    /// POST /vital        – set a player's health or power value
    static CommandResult SetVital(SetVitalParams const& params);

    /// POST /kick?name=…  – kick an online player (with optional reason)
    static CommandResult KickPlayer(std::string const& name, std::string const& reason = "");

private:
    // Shared JSON helpers (also used by HttpServer)
    static std::string BuildPlayerJson(struct Player const* player);
    static std::string JsonEscape(std::string const& s);
};