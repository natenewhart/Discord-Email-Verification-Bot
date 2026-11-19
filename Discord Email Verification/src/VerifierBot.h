// Nate Newhart - VerifierBot.h

#pragma once

#include <print> // TODO: remove all print includes
#include <format>

#include <dpp/dpp.h>
#include "EnvLoader.h"
#include "Verify.h"
#include "Mailer.h"
#include "GuildRegistry.h"

class VerifierBot
{
public:
	VerifierBot();
	bool GetStatus() const;

	void Start();

private:
	void OnReady(const dpp::ready_t& event);
	void OnSlashCommand(const dpp::slashcommand_t& event);

	// Slash Commands
	void HandleVerify(const dpp::slashcommand_t& event);
	bool IsVerified(const dpp::guild_member& member) const;

	void HandleConfirm(const dpp::slashcommand_t& event);
	void AddVerifiedRole(const dpp::slashcommand_t& event);

	static Mailer::Email ConstructEmail(std::string to, std::string code);
	const GuildRegistry::GuildData& GetGuild(dpp::snowflake id) const;

private:
	GuildRegistry mGuildData;
	bool mStatus;
	dpp::cluster mBot;
	Verify mVerifier;
	Mailer::SMTP mMailer;
};

