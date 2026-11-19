#include "GuildRegistry.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

GuildRegistry::GuildRegistry()
    : mFileStatus(true) // Default file loading parameter
{
    std::ifstream file("config.json");
    if (!file.is_open())
    {
        std::cerr << "Could not open the file!" << std::endl;
        mFileStatus = false;
    }
    try
    {
        json data = json::parse(file);

        for (const auto& [guildId, guild] : data["guilds"].items())
        {
            auto mailerType = Mailer::StringToType(guild["mailer"]["type"].get<std::string>());

            mGuildIdToConfigMap.emplace(ConvertJsonId(guildId), GuildData{
                guild["allowedDomains"].get<std::vector<std::string>>(),
                AllocateMailer(guild, mailerType),
                ConvertJsonId(guild["verifiedRoleId"]),
                mailerType
            });
        }
    }
    catch (const json::parse_error& e) {
     // Triggered by invalid JSON syntax (e.g., missing quotes or trailing commas)
        std::cerr << "JSON Syntax Error: " << e.what() << '\n'
            << "Byte position of error: " << e.byte << '\n';
    }
    catch (const json::out_of_range& e) {
     // Triggered when .at("key") cannot find the requested key
        std::cerr << "Missing Key Error: " << e.what() << '\n';
    }
    catch (const json::type_error& e) {
     // Triggered when the JSON value type doesn't match the C++ type
        std::cerr << "Type Mismatch Error: " << e.what() << '\n';
    }
    catch (const json::exception& e) {
     // Catch-all for any other nlohmann::json error
        std::cerr << "General JSON Error: " << e.what() << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
    }
}

bool GuildRegistry::GetStatus() const
{
    return mFileStatus;
}

const GuildRegistry::GuildData& GuildRegistry::Get(dpp::snowflake guildId) const
{
	return mGuildIdToConfigMap.at(guildId);
}

void GuildRegistry::AddGuild(dpp::snowflake event)
{
	return;
}

std::unique_ptr<const Mailer::IMailer> GuildRegistry::AllocateMailer(const json& guildJson, Mailer::Type type) const
{
    switch (type)
    {
        case Mailer::Type::Console:
        {
            return std::make_unique<Mailer::Console>();
            break;
        }
        case Mailer::Type::SMTP:
        {
            return std::make_unique<Mailer::SMTP>
            (
                guildJson["mailer"]["host"].get<std::string>(),
                guildJson["mailer"]["port"].get<unsigned int>(),
                guildJson["mailer"]["username"].get<std::string>(),
                guildJson["mailer"]["passwordEnv"].get<std::string>(),
                guildJson["mailer"]["displayName"].get<std::string>()
            );
            break;
        }
    }
}

dpp::snowflake GuildRegistry::ConvertJsonId(const json& jsonItem) const
{
    return dpp::snowflake(std::stoull(jsonItem.get<std::string>()));
}
