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

struct EmailMessage
{
	std::string to; // Recieving email address
	std::string subject;
	std::string body;
};

enum class SendStatus
{
	Pass
};

class IMailer
{
public:
	IMailer() = default;
	virtual ~IMailer() = default;

	virtual SendStatus Send(const EmailMessage& message) = 0; // Send email

};

// Simple console mailer which prints email to console
class ConsoleMailer : public IMailer
{
public:
	SendStatus Send(const EmailMessage& message) override;
};

class SMTPMailer : public IMailer
{
public:
	SendStatus Send(const EmailMessage& message) override;
};

