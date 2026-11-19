// Nate Newhart - GuildRegistry.h
// GuildRegistry object to store guildIds and per guild data

#pragma once

#include <memory>
#include <unordered_map>
#include <dpp/dpp.h>

#include "Mailer.h"
using json = nlohmann::json;

class GuildRegistry
{
public:
	struct GuildData // Per guild configuration variables
	{
		std::vector<std::string> domains;              // Valid email domains
		std::unique_ptr<const Mailer::IMailer> mailer; // Mailer object stored
		dpp::snowflake verifiedRoleId;
		Mailer::Type mailerType;                       // SMTP, Console, etc
	};

public:
	GuildRegistry();
	bool GetStatus() const;

	const GuildData& Get(dpp::snowflake guildId) const; // Return guild data given inputted guild id
	const auto& GetAll() const { return mGuildIdToConfigMap; }
	void AddGuild(dpp::snowflake event);

private:
	std::unique_ptr<const Mailer::IMailer> AllocateMailer(const json& guildJson, Mailer::Type type) const;
	dpp::snowflake ConvertJsonId(const json& jsonItem) const;

private:
	bool mFileStatus;
	std::unordered_map<dpp::snowflake, GuildData> mGuildIdToConfigMap;
	std::mutex mMutex; // Multiple servers & bot commands can access map, mutex for thread safety
};

