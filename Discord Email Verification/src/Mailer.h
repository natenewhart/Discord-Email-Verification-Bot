// Nate Newhart - Mailer.h
// Email sending object

#pragma once

#include <string>

class Mailer
{
	Mailer() = default;

	void Send(std::string to, std::string subject, std::string header);
};

