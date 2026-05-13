#include "SquadManager.h"

#include "CharacterDatabase.h"
#include "DatabaseEnv.h"
#include "Map.h"
#include "Player.h"
#include "QueryHolder.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include <chrono>
#include <thread>

namespace
{
    SquadManager g_SquadManager;

    WorldSession* CreateOfflineSession(uint32 accountId)
    {
        return new WorldSession(
            accountId, "", 0, nullptr, SEC_PLAYER,
            sWorld->getIntConfig(CONFIG_EXPANSION),
            0, LOCALE_enUS, 0, false, false, 0
        );
    }

    void SendLoginPackets(WorldSession* session, Player* player)
    {
        player->SendDungeonDifficulty(false);

        WorldPacket data(SMSG_LOGIN_VERIFY_WORLD, 20);
        data << player->GetMapId();
        data << player->GetPositionX();
        data << player->GetPositionY();
        data << player->GetPositionZ();
        data << player->GetOrientation();
        session->SendPacket(&data);

        session->SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

        data.Initialize(SMSG_FEATURE_SYSTEM_STATUS, 2);
        data << uint8(2);
        data << uint8(0);
        session->SendPacket(&data);

        data.Initialize(SMSG_LEARNED_DANCE_MOVES, 4 + 4);
        data << uint32(0);
        data << uint32(0);
        session->SendPacket(&data);

        if (player->HasUnitState(UNIT_STATE_LOGOUT_TIMER))
        {
            player->SetRooted(false, true, true);
            player->RemoveUnitFlag(UNIT_FLAG_STUNNED);
        }

        player->SendInitialPacketsBeforeAddToMap();

        player->GetMap()->SendInitTransports(player);
        player->GetMap()->SendInitSelf(player);

        player->GetObjectVisibilityContainer().CleanVisibilityReferences();
        player->UpdateObjectVisibility(false);

        player->CleanupChannels();
        player->SendInitialPacketsAfterAddToMap();

        uint32 currZone, currArea;
        player->GetZoneAndAreaId(currZone, currArea);
        player->SendInitWorldStates(currZone, currArea);
        player->SetInGameTime(GameTime::GetGameTimeMS().count());
    }
}

SquadManager* SquadManager::instance()
{
    return &g_SquadManager;
}

std::vector<Player*> SquadManager::GetSquad(Player* leader)
{
    auto it = _squads.find(leader->GetGUID().GetRawValue());
    if (it != _squads.end())
        return it->second;
    return {};
}

void SquadManager::DespawnSquad(Player* leader)
{
    auto guid = leader->GetGUID().GetRawValue();
    auto it = _squads.find(guid);
    if (it == _squads.end())
        return;

    for (Player* member : it->second)
    {
        if (!member)
            continue;

        member->SaveToDB(false, false);
        member->RemoveFromWorld();

        if (WorldSession* session = member->GetSession())
        {
            session->SetPlayer(nullptr);
            delete session;
        }

        delete member;
    }

    _squads.erase(it);
}

void SquadManager::LoadSquad(Player* leader)
{
    uint32 accountId = leader->GetSession()->GetAccountId();
    ObjectGuid::LowType leaderGuid = leader->GetGUID().GetCounter();

    QueryResult result = CharacterDatabase.Query(
        "SELECT guid FROM characters "
        "WHERE account = {} AND guid != {}",
        accountId, leaderGuid
    );

    if (!result)
        return;

    std::vector<Player*> squad;
    float angle = 0.0f;
    uint32 count = 0;

    do
    {
        uint32 guidLow = result->Fetch()[0].Get<uint32>();
        ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(guidLow);

        WorldSession* session = CreateOfflineSession(accountId);
        Player* companion = new Player(session);

        auto holder = std::make_shared<CharacterDatabaseQueryHolder>();
        holder->SetSize(MAX_PLAYER_LOGIN_QUERY);

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
        stmt->SetData(0, guidLow);
        holder->SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

        bool ready = false;
        auto callback = CharacterDatabase.DelayQueryHolder(holder);
        callback.AfterComplete([&ready](SQLQueryHolderBase const&) {
            ready = true;
        });

        while (!ready)
        {
            if (callback.InvokeIfReady())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        if (!companion->LoadFromDB(guid, *holder))
        {
            delete companion;
            delete session;
            continue;
        }

        session->SetPlayer(companion);

        float x = leader->GetPositionX() + 3.0f * std::cos(angle);
        float y = leader->GetPositionY() + 3.0f * std::sin(angle);
        companion->Relocate(x, y, leader->GetPositionZ(), leader->GetOrientation());

        companion->SetFaction(leader->GetFaction());
        companion->SetPvP(false);

        if (!leader->GetMap()->AddPlayerToMap(companion))
        {
            session->SetPlayer(nullptr);
            delete companion;
            delete session;
            continue;
        }

        squad.push_back(companion);
        angle += float(M_PI) * 0.66f;
        ++count;

    } while (result->NextRow());

    _squads[leader->GetGUID().GetRawValue()] = squad;
}

void SquadManager::SwapTo(Player* current, ObjectGuid const& targetGuid)
{
    uint32 accountId = current->GetSession()->GetAccountId();

    auto it = _squads.find(current->GetGUID().GetRawValue());
    if (it == _squads.end())
        return;

    // Find target companion
    Player* target = nullptr;
    for (Player* member : it->second)
    {
        if (member && member->GetGUID() == targetGuid)
        {
            target = member;
            break;
        }
    }

    if (!target)
        return;

    if (!target->IsInWorld() || !target->FindMap())
        return;

    WorldSession* onlineSession = current->GetSession();
    WorldSession* offlineSession = target->GetSession();

    // Save current character
    current->SaveToDB(false, false);
    current->RemoveFromWorld();

    // Detach both from their sessions
    onlineSession->SetPlayer(nullptr);
    offlineSession->SetPlayer(nullptr);

    // Swap sessions
    current->SetSession(offlineSession);
    offlineSession->SetPlayer(current);

    target->SetSession(onlineSession);
    onlineSession->SetPlayer(target);

    // Recreate talk class with new session
    delete target->PlayerTalkClass;
    target->PlayerTalkClass = new PlayerMenu(target->GetSession());

    // Send all login init packets so the client loads this character
    SendLoginPackets(onlineSession, target);

    // Swap squad bookkeeping: key the squad under the new active character
    _squads[target->GetGUID().GetRawValue()] = it->second;
    _squads.erase(it);
}
