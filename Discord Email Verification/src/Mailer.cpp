#include "Mailer.h"
#include <print>

// ---------------------- IMailer ----------------------

// ---------------------- Console Mailer ----------------------

SendStatus ConsoleMailer::Send(const EmailMessage& message)
{
	std::println("\nTo: {}\nSubject: {}\n\n{}", message.to, message.subject, message.body);
	return SendStatus::Pass;
}

// ---------------------- SMTP Mailer ----------------------

SendStatus SMTPMailer::Send(const EmailMessage& message)
{
	return SendStatus();
}
