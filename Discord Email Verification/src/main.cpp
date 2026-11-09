// Nate Newhart - main.cpp

#include <print> // TODO: remove all print includes
#include <iostream>
#include <format>

#include <dpp/dpp.h>
#include "EnvLoader.h"
#include "Verify.h"

/*
* TODO:
* 
* FINAL PIPELINE:
* user does /start in the verify channel which sends a message with a button
* They pin this message because this is the verification message
* 
* /start sends message vith verifcication button
* verification button creates a popup for your email address
* user enters email address and clicks submit
*
* Verification email then sent
* 
* popup now shoes input box for a n-digit code with a submit button
* 
* Successful verification leads to popup saying "You have been verified!" and the user is given the verified role
* 
* SIMPLE PIPELINE:
* * user does /start in the verify channel which sends a message with instructions to verify their email address
* 
* /verify command is used with the email address as an argument
* Verification email then sent to there email if it is correct domain and correctly formatted
* 
* popup now shoes input box for a n-digit code with a submit button
* 
* Successful verification leads to popup saying "You have been verified!" and the user is given the verified role
* 
* TODO:
* add guild config json which is updated with /config command and stored on disk for data per server
* 
* // SECURITY TODO:
* 1. cyptographically secure RNG or CSPRNG
* 2. timeouts for multiple users trying to verify at the same time. Prevents server attacks from tons of users trying to apply at the same time
*/

int main()
{
	if (!Env::load()) return -1; // Load environment variables from .env file
	Verify verifier(Env::getVar("EMAIL_DOMAIN")); // Create an instance of the Verify class

	const std::string cBotToken = Env::getVar("BOT_TOKEN");

	dpp::cluster bot(cBotToken); 
	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event)
	{
		if (dpp::run_once<struct register_bot_commands>())
		{
			dpp::slashcommand verify("verify", "Enter your email", bot.me.id);
			verify.add_option(dpp::command_option(dpp::co_string, "email", "Your email address", true));

			dpp::slashcommand confirm("confirm", "Enter the code you received by email", bot.me.id);
			confirm.add_option(dpp::command_option(dpp::co_string, "code", "The code from your email", true));
			// TODO: code max length of 256 or whatever so no one sends mb of text through api

			bot.guild_bulk_command_create({ verify, confirm }, // TODO: change this for official launch
										  dpp::snowflake(Env::getVar("GUILD_ID")));
		}
	});
	
	// TODO: check if user is alreadyd verifed if they are they cant do it again
	// TODO: add expires in 3m message on email
	bot.on_slashcommand([&verifier](const dpp::slashcommand_t& event)
	{
		auto userId = event.command.usr.id;
		if (event.command.get_command_name() == "verify")
		{
			auto userEmail = std::get<std::string>(event.get_parameter("email"));
			switch (verifier.SubmitUser(userId, userEmail))
			{
			case Verify::SubmissionResults::Pass:
				std::println("Code: {}", verifier.GetVerificationCode(userId));
				event.reply(dpp::message("Check your inbox, then run /confirm <code>.")
							.set_flags(dpp::m_ephemeral)); // Hidden message only to user
				break;
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
		if (event.command.get_command_name() == "confirm")
		{
			std::string verificationCode = std::get<std::string>(event.get_parameter("code"));
			switch (verifier.ConfirmUser(userId, verificationCode))
			{
			case Verify::ConfirmationResults::Pass:
				event.reply(dpp::message("Confirmation successful!").set_flags(dpp::m_ephemeral));
				break;
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
	});

	dpp::snowflake guildId = std::stoull(Env::getVar("GUILD_ID"));

	bot.start(dpp::st_wait);

	return 0;
}

// TODO: timed removal of submissions, after leastmost submission set a 30 minute timer and after its over remove all submissions
