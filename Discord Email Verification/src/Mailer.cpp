#include "Mailer.h"

#include <print>
#include <array>
#include <smtpclient/cpp/plaintextmessage.hpp>
#include <smtpclient/cpp/forcedsecuresmtpclient.hpp>
#include <smtpclient/cpp/opportunisticsecuresmtpclient.hpp>

namespace Mailer
{
    // ---------------------- General ----------------------
	namespace
	{
		constexpr std::array scStringToMailerType = // Static array map for "string" -> "Mailer::Type"
		{
			std::pair{std::string_view{"console"}, Mailer::Type::Console},
			std::pair{std::string_view{"smtp"}, Mailer::Type::SMTP}
		};
	}
	Type StringToType(std::string_view type)
	{
		for (const auto& [name, value] : scStringToMailerType)
		{
			if (name == type)
				return value;
		}

		throw std::invalid_argument("Unknown mailer type: " + std::string(type));
	}

    // ---------------------- Console Mailer ----------------------

	SendResult Console::Send(const Email& message) const
	{
		std::println("\nTo: {}\nSubject: {}\n\n{}", message.to, message.subject, message.body);
		return { SendStatus::Pass, "" };
	}

	// ---------------------- SMTP Mailer ----------------------

	SMTP::SMTP(const std::string& host, unsigned int port, const std::string& username, const std::string& password, const std::string& displayName)
		: mHost(host)
		, mPort(port)
		, mUsername(username)
		, mPassword(password)
		, mDisplayName(displayName)
	{}

	SendResult SMTP::Send(const Email& message) const
	{
		using namespace jed_utils::cpp;
		try
		{
			OpportunisticSecureSMTPClient client(mHost, mPort); // port 587: starts plain, upgrades to TLS
			client.setCredentials(Credential(mUsername, mPassword));

			PlaintextMessage msg(MessageAddress(mUsername, mDisplayName),
								 { MessageAddress(message.to, "") },
								 message.subject,
								 message.body);

			int error = client.sendMail(msg);
			auto sendStatus = ParseErrorNumber(error);
			return { sendStatus, client.getErrorMessage(error) + " (code " + std::to_string(error) + ")" };
		}
		catch (const std::exception& err) // bad addresses throw std::invalid_argument
		{
			return { SendStatus::ServerError, std::string(err.what()) + " std::exception thrown" };
		}
	}

	SendStatus SMTP::ParseErrorNumber(int error) const
	{
		switch (error)
		{
		case 0:   return SendStatus::Pass;
		case -87: return SendStatus::UserError; // RCPT TO rejected: the recipient address was refused
		default:  return SendStatus::ServerError;
		}
	}
}