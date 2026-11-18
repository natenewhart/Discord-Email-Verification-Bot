#include "VerifierBot.h"

VerifierBot::VerifierBot()
	: mStatus(Env::load())
	, mBot(Env::getVar("BOT_TOKEN"))
	, mMailer(Env::getVar("SMTP_HOST", "smtp.gmail.com"),
			  static_cast<unsigned int>(std::stoul(Env::getVar("SMTP_PORT", "587"))),
			  Env::getVar("SMTP_USERNAME"),
			  Env::getVar("SMTP_PASSWORD"),
			  Env::getVar("DISPLAY_NAME", "Email Verifier"))
	, mVerifier(Env::getVar("EMAIL_DOMAIN"))
{
	mBot.on_log(dpp::utility::cout_logger());
	mBot.on_ready([this](const dpp::ready_t& event) { OnReady(event); });
	mBot.on_slashcommand([this](const dpp::slashcommand_t& event) { OnSlashCommand(event); });
}

bool VerifierBot::GetStatus() const
{
	return mStatus;
}

void VerifierBot::Start()
{
	mBot.start(dpp::st_wait);
}

// ------------------- Main -------------------

void VerifierBot::OnReady(const dpp::ready_t& event)
{
	if (dpp::run_once<struct register_bot_commands>())
	{
		dpp::slashcommand verify("verify", "Enter your email", mBot.me.id);
		verify.add_option(dpp::command_option(dpp::co_string, "email", "Your email address", true));

		dpp::slashcommand confirm("confirm", "Enter the code you received by email", mBot.me.id);
		confirm.add_option(dpp::command_option(dpp::co_string, "code", "The code from your email", true));

		mBot.guild_bulk_command_create({ verify, confirm }, // TODO: change this for official launch
									   dpp::snowflake(Env::getVar("GUILD_ID")));
	}
}

void VerifierBot::OnSlashCommand(const dpp::slashcommand_t & event)
{
	const std::string name = event.command.get_command_name();
	if (name == "verify")  HandleVerify(event);
	if (name == "confirm") HandleConfirm(event);
}

void VerifierBot::HandleVerify(const dpp::slashcommand_t& event)
{
	auto userId = event.command.usr.id;
	auto userEmail = std::get<std::string>(event.get_parameter("email"));

	if (IsVerified(event.command.member)) // Pass when user already has verified role
	{
		event.reply(dpp::message("You are already verified.")
					.set_flags(dpp::m_ephemeral)); // Hidden message only to user
	}

	std::string verificationCode = "fallback"; // Verification code given by SubmitUser
	switch (mVerifier.SubmitUser(userId, userEmail, verificationCode))
	{
	case Verify::SubmissionResults::Pass:
	{
		event.thinking(true);

		Mailer::SendResult emailResult = mMailer.Send(ConstructEmail(userEmail, verificationCode));

		if (emailResult.sendStatus == Mailer::SendStatus::Pass)
		{
			event.edit_original_response(dpp::message(
				"Check your inbox, then run /confirm <code>. If it doesn't arrive in a few minutes, "
				"check the spelling of your address and run /verify again."));
		}
		else
		{
			mVerifier.RemoveUser(userId);
			mBot.log(dpp::ll_error, "email send failed: " + emailResult.errorMessage);
			event.edit_original_response(dpp::message(
				"Couldn't send the email. Please try again later."));
		}
		break;
	}
	case Verify::SubmissionResults::CooldownFailure:
		event.reply(dpp::message("Verification cooldown, wait 20s between /verify calls")
					.set_flags(dpp::m_ephemeral)); // Hidden message only to user
		break;
	case Verify::SubmissionResults::EmailDomainError:
		event.reply(dpp::message("Verification Failure: email domain is invalid")
					.set_flags(dpp::m_ephemeral)); // Hidden message only to user
		break;
	case Verify::SubmissionResults::EmailNameError:
		event.reply(dpp::message("Verification Failure: email name error")
					.set_flags(dpp::m_ephemeral)); // Hidden message only to user
		break;
	}
}

bool VerifierBot::IsVerified(const dpp::guild_member& member) const
{
	const auto& roles = member.get_roles();
	return std::find(roles.begin(), roles.end(), dpp::snowflake(Env::getVar("VERIFIED_ROLE_ID"))) != roles.end();
}

void VerifierBot::HandleConfirm(const dpp::slashcommand_t & event)
{
	auto userId = event.command.usr.id;
	std::string verificationCode = std::get<std::string>(event.get_parameter("code"));

	switch (mVerifier.ConfirmUser(userId, verificationCode))
	{
	case Verify::ConfirmationResults::Pass: // TODO: add roll creation here for passing
	{
		event.reply(dpp::message("Confirmation successful!").set_flags(dpp::m_ephemeral));
		AddVerifiedRole(event);
		break;
	}
	case Verify::ConfirmationResults::UserNotFound:
		event.reply(dpp::message("No active code. Codes expire after 3 minutes, so use /verify to start over.")
					.set_flags(dpp::m_ephemeral));
		break;
	case Verify::ConfirmationResults::WrongCode:
		event.reply(dpp::message("Incorrect code.").set_flags(dpp::m_ephemeral));
		break;
	case Verify::ConfirmationResults::TooManyAttempts:
		event.reply(dpp::message("Too many attempts. Start again with /verify.")
					.set_flags(dpp::m_ephemeral));
		break;
	}
}

void VerifierBot::AddVerifiedRole(const dpp::slashcommand_t& event)
{
	auto userId = event.command.usr.id;
	dpp::snowflake guildId = event.command.guild_id;
	dpp::snowflake verifiedRoleId = Env::getVar("VERIFIED_ROLE_ID"); // Replace with your Role ID

	mBot.guild_member_add_role(guildId, userId, verifiedRoleId, [event, this](const dpp::confirmation_callback_t& callback)
	{
		if (callback.is_error())
		{
			mBot.log(dpp::ll_error, "Failed to add role: " + callback.get_error().message);
			//event.reply("Failed to add role: " + callback.get_error().message);
		}
		//else
		//{
		//	event.reply("Role added successfully!");
		//}
	});
}

// ------------------- Helpers -------------------

Mailer::Email VerifierBot::ConstructEmail(std::string to, std::string code)
{
	Mailer::Email email;
	email.to      = to;
	email.subject = "Discord Verification Code";
	email.body    = "Your verification code is " + code + "\nThis code expires in 3m.";
	return email;
}
