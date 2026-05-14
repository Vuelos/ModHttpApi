#pragma once

#include <string>

struct CommandResult
{
    bool        ok         = true;
    int         httpStatus = 200;
    std::string value;
    std::string error;

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
