#pragma once

#include <optional>
#include <string>

#include "libs/json.hpp"

using json = nlohmann::json;

inline std::optional<float> TryParseFloat(std::string const& s)
{
    if (s.empty()) return std::nullopt;
    try { return std::stof(s); } catch (...) { return std::nullopt; }
}

inline std::optional<uint32_t> TryParseUint(std::string const& s)
{
    if (s.empty()) return std::nullopt;
    try { return static_cast<uint32_t>(std::stoul(s)); } catch (...) { return std::nullopt; }
}

inline std::optional<int32_t> TryParseInt(std::string const& s)
{
    if (s.empty()) return std::nullopt;
    try { return std::stoi(s); } catch (...) { return std::nullopt; }
}

inline std::optional<json> ParseJsonBody(std::string const& body)
{
    if (body.empty()) return std::nullopt;
    try { return json::parse(body); } catch (...) { return std::nullopt; }
}
