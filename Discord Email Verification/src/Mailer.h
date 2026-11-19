// Nate Newhart - Mailer.h
// Virtual email sending parent 

// TODO
/*
* Make IMailer as the parent mailer class as purely virtual
* 
* Make mailer children classes which mail using different methods, one being the console and one using gmail, etc
* 
* Consider wrapper namespace so that all structs and enum classes sit under namespace
*/

#pragma once

#include <string>
#include <unordered_map>

namespace Mailer
{
	enum class Type
	{
		Console,
		SMTP
	};
	Type StringToType(std::string_view type); // Convert mailer type string to type [EX: "smtp" -> Type::SMTP]

	struct Email
	{
		std::string to; // Receiving email address
		std::string subject;
		std::string body;
	};

	enum class SendStatus // Email send status results
	{
		Pass,        // Successful send
		UserError,   // Error from bad to address (receiving email address)
		ServerError  // Server side error from host, from address, password, etc
	};

	struct SendResult // Result of send function
	{
		SendStatus  sendStatus;
		std::string errorMessage;
	};

	class IMailer
	{
	public:
		IMailer() = default;
		virtual ~IMailer() = default;

		virtual SendResult Send(const Email& message) const = 0; // Send email
	};

	// Simple console mailer which prints email to console
	class Console : public IMailer
	{
	public:
		SendResult Send(const Email& message) const override;
	};

	// TODO: move to new file because this uses a complete isolated library so should be isolated
	class SMTP : public IMailer
	{
	public:
		SMTP(const std::string& host, unsigned int port, const std::string& username,
			 const std::string& password, const std::string& displayName = "");

		SendResult Send(const Email& message) const override;
	private:
		SendStatus ParseErrorNumber(int error) const; // Return correct send status based on SMTP error number

	private:
		unsigned int mPort;
		std::string mHost;
		std::string mUsername;
		std::string mPassword;
		std::string mDisplayName;
	};
}

