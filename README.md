# ModHttpApi

HTTP REST API for AzerothCore — a DM companion app interface exposing server state, player data, world control, and NPC/creature/gameobject management over HTTP.

## Features

### Server & World
- Server status, uptime, session counts
- Game time, terrain height, zone/area lookup
- List all maps from DBC, find nearby entities on a map
- Set zone weather (rain, snow, storm, etc.)

### Player Queries
- Online player lookup (basic info, stats, skills, talents, equipment)
- Player inventory (backpack, bags, bank)
- Player quests (active + completed with objectives)

### Player Management
- Teleport players, set health/power, kick
- Give/remove items
- Toggle GM mode, reset cooldowns, morph display ID
- Force-complete quests

### Creature & NPC
- Spawn temporary or permanent creatures
- Despawn creatures by GUID
- Creature template lookup (level, faction, loot, models, etc.)
- Make any spawned NPC say, yell, or emote text

### GameObjects
- Spawn and despawn game objects

### Chat
- System-wide broadcasts and positional say/yell

## Installation

1. Clone into your `modules/` directory:

```bash
cd modules
git clone https://github.com/your-repo/ModHttpApi
```

2. Re-run CMake with `-DMODULES=static` (module is auto-discovered).

3. Build and install.

## Configuration

Edit `conf/ModHttpApi.conf.dist` (copied to `conf/ModHttpApi.conf` on first run):

```ini
[worldserver]

# Enable the HTTP API server
HttpApi.Enable = 1

# Port to listen on
HttpApi.Port = 8080

# Bind address (0.0.0.0 = all interfaces)
HttpApi.BindAddress = 0.0.0.0
```

## API Reference

All requests and responses use `Content-Type: application/json`. Errors return `{"error": "<message>"}` with an appropriate HTTP status code.

### Server Info

**`GET /server`** — Server status, uptime, session counts.

Response:
```json
{
  "uptime": 3600,
  "gameTime": 1714000000,
  "startTime": 1713996400,
  "activeSessions": 3,
  "queuedSessions": 0,
  "playerCount": 3,
  "maxPlayerCount": 50,
  "maxActiveSessions": 5,
  "maxQueuedSessions": 0,
  "playerLimit": 100
}
```

### Time

**`GET /time`** — Server uptime and game timestamps.

```json
{"uptime": 3600, "gameTime": 1714000000, "startTime": 1713996400}
```

### Ground Height

**`GET /groundz?map=<uint>&x=<float>&y=<float>`** — Terrain Z at a coordinate.

```json
{"z": 83.53}
```

### Zone Info

**`GET /zone?map=<uint>&x=<float>&y=<float>&z=<float>`** — Zone and area at a coordinate.

```json
{"zoneId": 12, "zoneName": "Elwynn Forest", "areaId": 89, "areaName": "Northshire"}
```

### Maps List

**`GET /maps`** — All maps from DBC.

```json
[
  {"id": 0, "name": "Eastern Kingdoms", "type": 0, "isDungeon": false, "isRaid": false, "instanceable": false, "maxPlayers": 0, "expansion": 0},
  {"id": 1, "name": "Kalimdor", "type": 0, "isDungeon": false, "isRaid": false, "instanceable": false, "maxPlayers": 0, "expansion": 0}
]
```

### Nearby Entities

**`GET /nearby?map=<uint>&x=<float>&y=<float>&range=<float>`** — Creatures and game objects near a point.

```json
{
  "creatures": [
    {"guid": "0x000000000000002A", "entry": 6, "name": "Kobold Vermin", "x": -8949.95, "y": -132.49}
  ],
  "gameobjects": [],
  "total": 1
}
```

### Player

**`GET /player?name=<string>`** — Basic online player info.

```json
{
  "name": "Arthas",
  "guid": "0x000000000000002A",
  "mapId": 0,
  "x": -8949.95,
  "y": -132.49,
  "z": 83.53,
  "orientation": 0.0,
  "level": 80,
  "race": 1,
  "class": 1,
  "gender": 0,
  "health": 10000,
  "maxHealth": 10000,
  "powerType": 1,
  "power": 5000,
  "maxPower": 5000,
  "zoneId": 12,
  "areaId": 89,
  "isGM": false,
  "guildId": 0
}
```

### Player Stats

**`GET /player/stats?name=<string>`** — Detailed stats, resistances, combat ratings.

### Player Skills

**`GET /player/skills?name=<string>`** — Passive skills, talents, castable spells.

### Player Quests

**`GET /player/quests?name=<string>`** — Active and completed quests with objectives.

### Player Equipment

**`GET /player/equipment?name=<string>`** — All 19 equipped item slots with full item data.

### Player Inventory

**`GET /player/inventory?name=<string>`** — Backpack items, equipped bags with contents, bank items and bank bags.

```json
{
  "backpack": [{"slot": 0, "item": {...}}],
  "bags": [{"bagSlot": 0, "bagEntry": 12345, "bagSlots": 16, "contents": [...]}],
  "bank": {"items": [...], "bags": [...]}
}
```

### Players List

**`GET /players?map=<uint>&x=<float>&y=<float>&range=<float>`** — All online players, optionally filtered by position.

```json
[{"name": "Arthas", ...}]
```

### Teleport

**`POST /teleport`** — Teleport a player.

```json
// Request
{"name": "Arthas", "mapId": 1, "x": 1629.0, "y": -4411.0, "z": 38.7, "orientation": 0}
// Response
{"teleported": true}
```

### Give / Remove Item

**`POST /item/give`** — Give (positive count) or remove (negative count) items.

```json
// Give 5 Hearthstones
{"name": "Arthas", "itemEntry": 6948, "count": 5}
// Response
{"player": "Arthas", "itemEntry": 6948, "count": 5}
```

### Set Vital

**`POST /vital`** — Set health or power to a value (-1 = full).

```json
{"name": "Arthas", "vitalType": "health", "value": -1}
// Response
{"set": true}
```

### Kick

**`POST /kick`** — Disconnect a player.

```json
{"name": "Griefer", "reason": "AFK farming exploit"}
// Response
{"kicked": true}
```

### Toggle GM

**`POST /player/gm`** — Enable or disable GM mode.

```json
{"name": "Arthas", "gm": true}
// Response
{"gm": true, "name": "Arthas"}
```

### Reset Cooldowns

**`POST /player/cooldowns/reset`** — Reset all spell cooldowns.

```json
{"name": "Arthas"}
// Response
{"cooldownsReset": true, "name": "Arthas"}
```

### Morph

**`POST /player/morph`** — Change player display ID.

```json
{"name": "Arthas", "displayId": 12345}
// Response
{"morphed": true, "name": "Arthas", "displayId": 12345}
```

### Complete Quest

**`POST /player/quest/complete`** — Force-complete a quest for a player.

```json
{"name": "Arthas", "questId": 1234}
// Response
{"completed": true, "questId": 1234, "name": "Arthas"}
```

### Broadcast

**`POST /broadcast`** — Send system, say, or yell messages.

```json
// System notification
{"message": "Restart in 5 minutes.", "type": "system"}
// Positional say
{"message": "Hail, adventurer!", "type": "say", "mapId": 0, "x": -8949.95, "y": -132.49, "z": 83.53}
// Response
{"sent": true}
```

### Spawn Creature

**`POST /spawn`** — Spawn a creature. Pass `despawnSecs` for temporary spawns.

```json
{"entry": 400, "mapId": 0, "x": -8945.0, "y": -130.0, "despawnSecs": 300}
// Response
{"guid": "0x000000000000002A", "guid_raw": 42, "entry": 400, "mapId": 0, "x": -8945.0, "y": -130.0, "z": 82.1}
```

### Despawn Creature

**`DELETE /spawn/{guid}`** — Despawn a creature by its raw 64-bit GUID.

```json
// Response
{"despawned": true}
```

### Spawn GameObject

**`POST /gameobject/spawn`** — Spawn a game object.

```json
{"entry": 1234, "mapId": 0, "x": -8945.0, "y": -130.0, "z": 82.0, "orientation": 1.5, "despawnSecs": 120}
// Response
{"guid": "0x000000000000003A", "guid_raw": 58, "entry": 1234, "mapId": 0, "x": -8945.0, "y": -130.0, "z": 82.0}
```

### Despawn GameObject

**`DELETE /gameobject/spawn/{guid}`** — Despawn a game object by its raw 64-bit GUID.

```json
// Response
{"despawned": true}
```

### NPC Info

**`GET /npc/{entry}`** — Creature template info.

```json
{
  "entry": 400,
  "name": "Kobold Vermin",
  "subname": "",
  "minlevel": 1,
  "maxlevel": 2,
  "faction": 14,
  "npcflag": 0,
  "rank": 0,
  "type": 7,
  "models": [233, 234],
  "speed_run": 1.14,
  "health_mod": 1.0,
  "lootid": 400
}
```

### NPC Say / Yell / Emote

**`POST /npc/say`** — Make a spawned creature speak.

```json
{"guid": 42, "text": "Hello, $N!", "type": "say"}
// type can be: "say", "yell", "emote"
// Response
{"spoken": true}
```

### Weather

**`POST /weather`** — Set weather in a zone.

```json
{"zoneId": 12, "weatherType": 3, "intensity": 0.8}
// weatherType: 0=fine, 1=rain, 2=snow, 3=storm, 86=thunder, 90=black rain
// intensity: 0.0 (light) to 1.0 (heavy)
// Response
{"weatherSet": true, "zoneId": 12, "weatherType": 3, "intensity": 0.8}
```

## Development

The module is auto-discovered by the AzerothCore build system — no `CMakeLists.txt` needed. Source is in `src/` organized as:

```
src/
├── ModHttpServer.cpp          # Entry point (WorldScript)
├── common/                    # Shared types (CommandResult, JSON helpers)
├── server/                    # HTTP server (cpp-httplib + routes)
├── controllers/
│   ├── server/                # Server info, time, ground, zone
│   ├── player/                # Player queries + management
│   ├── chat/                  # Broadcast, NPC dialogue
│   ├── item/                  # Item give/remove
│   ├── creature/              # Creature spawn/despawn
│   ├── gameobject/            # GameObject spawn/despawn
│   ├── npc/                   # NPC template info + speech
│   └── world/                 # Maps, nearby, weather
├── models/                    # Request param structs
└── libs/                      # cpp-httplib, nlohmann/json
```

Each domain follows a Controller → Repository pattern:
- **Controller**: validates params, calls Repository, returns `CommandResult`
- **Repository**: wraps AzerothCore API, returns `json` data
