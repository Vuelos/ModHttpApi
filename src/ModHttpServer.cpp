#include "ScriptMgr.h"
#include "HttpServer/HttpServer.h"

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

// Module entry point — registers all scripts
void AddModScripts()
{
    new HttpApiWorldScript();
}

// CMake compat forwarder
void AddModHttpServerScripts()
{
    AddModScripts();
}