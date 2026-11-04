// Nate Newhart - main.cpp

#include <print>
#include <iostream>

#include <dpp/dpp.h>
#include <CRandom/CRandom.hpp>
#include "EnvLoader.h"

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
*/

int main()
{
	if (!Env::load()) return -1; // Load environment variables from .env file
	
	const std::string cBotToken = Env::getVar("BOT_TOKEN");

	dpp::cluster bot(cBotToken); 
	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event)
	{
		if (dpp::run_once<struct register_bot_commands>())
		{
			bot.global_command_create(dpp::slashcommand("verify", "Input Verification Email", bot.me.id));
		}
	});
	
	bot.on_slashcommand([](const dpp::slashcommand_t& event)
	{
		if (event.command.get_command_name() == "verify")
		{

		}
	});

	bot.start(dpp::st_wait);

	return 0;
}