#include "Mailer.h"

#include <print>
#include <smtpclient/cpp/plaintextmessage.hpp>
#include <smtpclient/cpp/forcedsecuresmtpclient.hpp>
#include <smtpclient/cpp/opportunisticsecuresmtpclient.hpp>

namespace Mailer
{
    // ---------------------- IMailer ----------------------


    // ---------------------- Console Mailer ----------------------

	SendResult Console::Send(const Email& message)
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

	SendResult SMTP::Send(const Email& message)
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