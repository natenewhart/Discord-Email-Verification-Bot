// Nate Newhart - main.cpp

/*
* Library TODO notes:
* Possibly dont use cpr or the json library by the end of project if not needed
*
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
* Add verify roll FIRST which updates when you are verified on said server
* Add is verified check so no one can get verified twice
* 
* Add json data base per server
* hash map store guildId -> {server settings, MailerObject} and the mailer object stores mailer specific data (SMTP variables etc)
* This way each server can have a different IMailer object from there choosing, and store their variables on the stack
*
* 
* // SECURITY TODO:
* 1. timeouts for multiple users trying to verify at the same time. Prevents server attacks from tons of users trying to apply at the same time
*/

#include <print> // TODO: remove all print includes in whole file and prinlns
#include <iostream>
#include <format>

#include "VerifierBot.h"

int main()
{
	VerifierBot bot;
	if (!bot.GetStatus()) return -1; // Env failure

	bot.Start();
	return 0;
}
// TODO: timed removal of submissions, after least most submission set a 30 minute timer and after its over remove all submissions

int TestGuildRegistry()
{
    GuildRegistry gr;
    if (!gr.GetStatus()) return -1; // Config load failure

    const auto& guilds = gr.GetAll();

    std::println("\n========== GUILD REGISTRY ==========");
    std::println("Total Guilds: {}", guilds.size());

    for (const auto& [guildId, guild] : guilds)
    {
        std::println("\n-----------------------------------");
        std::println("Guild ID: {}", static_cast<uint64_t>(guildId));

        std::println("Allowed Domains:");
        for (const auto& domain : guild.domains)
        {
            std::println("  - {}", domain);
        }

        std::println("Verified Role ID: {}",
                     static_cast<uint64_t>(guild.verifiedRoleId));

        std::println("Mailer Type: {}",
                     guild.mailerType == Mailer::Type::SMTP ? "SMTP" : "Console");

        std::println("Mailer Initialized: {}",
                     guild.mailer ? "Yes" : "No");
    }

    std::println("\n========== END REGISTRY ==========");
    return 0;
}