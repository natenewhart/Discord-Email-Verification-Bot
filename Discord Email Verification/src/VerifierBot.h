// Nate Newhart - VerifierBot.h

#pragma once

#include <print> // TODO: remove all print includes
#include <format>
#include <dpp/dpp.h>
#include "EnvLoader.h"
#include "Verify.h"
#include "Mailer.h"

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

private:
	bool mStatus;
	dpp::cluster mBot;
	Mailer::SMTP mMailer;
	Verify mVerifier;
};

