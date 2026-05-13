#include "ScriptMgr.h"
#include "HttpServer.h"

class HttpApiWorldScript : public WorldScript
{
public:
    HttpApiWorldScript()
        : WorldScript("HttpApiWorldScript")
    {
    }

    void OnStartup() override
    {
        HttpServer::Start();
    }
};

void AddModHttpApiScripts()
{
    new HttpApiWorldScript();
}