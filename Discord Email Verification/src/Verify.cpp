#include "Verify.h"
#include <format>
#include <print>

Verify::Verify(const std::string& domain)
	: mcValidDomains{ domain }
{}

Verify::Verify(const std::vector<std::string>& domains)
	: mcValidDomains(domains)
{}

Verify::SubmissionResults Verify::SubmitUser(tUserId userId, std::string email)
{
	RemoveExpiredSubmissions();
	if (IsSubmitted(userId))
	{
		auto now = std::chrono::steady_clock::now();
		if (now - mUserIdToVerificationData[userId].timestamp < scSubmissionCooldown) // Cooldown failure
			return SubmissionResults::CooldownFailure;
		
		RemoveUser(userId); // Overwrite user when they double submit
	}

	// TODO: email cooldown doesnt apply if verification doesnt work; cooldown is only applied if you successfully submitted
	TrimString(email);
	auto emailResults = VerifyEmail(email);
	if (emailResults != SubmissionResults::Pass) return emailResults; // Return error value from email verification

	AddUser(userId, email);
	return SubmissionResults::Pass;
}

Verify::ConfirmationResults Verify::ConfirmUser(tUserId userId, const std::string& code)
{
	RemoveExpiredSubmissions();
	if (!IsSubmitted(userId)) return ConfirmationResults::UserNotFound; // User is not in system so fail

	std::string normCode = Verify::ToUpper(code);
	if (mUserIdToVerificationData[userId].verificationCode == normCode)
	{
		RemoveUser(userId);
		return ConfirmationResults::Pass;
	}

	mUserIdToVerificationData[userId].confirmationAttempts++;
	if (mUserIdToVerificationData[userId].confirmationAttempts == scMaxConfirmationAttempts)
	{
		RemoveUser(userId);
		return ConfirmationResults::TooManyAttempts;
	}

	return ConfirmationResults::WrongCode;
}

std::string Verify::GetVerificationCode(tUserId userId) const
{
	// TODO: make this safe from calling when user is not in system
	// uses .at() so code fails

	return mUserIdToVerificationData.at(userId).verificationCode; // Return the verification code for the user
}

bool Verify::IsSubmitted(tUserId userId) const
{
	return mUserIdToVerificationData.find(userId) != mUserIdToVerificationData.end();
}

// Private Helpers

std::string Verify::GenerateRandomCode()
{
	static_assert(scCodeLength >= 1 && scCodeLength <= 15, "code length out of range"); // Code length must match bounds
	
	static thread_local std::random_device rd;
	uint64_t bitLength = (1ULL << (4 * scCodeLength)) - 1;
	const uint64_t hi = rd();
	const uint64_t lo = rd();
	const uint64_t rand64 = (hi << 32) | lo;
	const uint64_t bits = rand64 & bitLength; // Preserve bottom bits to be converted to hexadecimal value

	return std::format("{:0{}X}", bits, scCodeLength);
}

void Verify::RemoveExpiredSubmissions()
{
	const auto now = std::chrono::steady_clock::now();

	while (!mTimeQueue.empty() &&
		   now - mTimeQueue.front().timestamp > scVerificationLifetime)
	{
		mUserIdToVerificationData.erase(mTimeQueue.front().userId);
		mTimeQueue.pop_front();
	}
}

void Verify::AddUser(tUserId userId, const std::string& email)
{
	auto code = GenerateRandomCode();
	auto now = std::chrono::steady_clock::now();
	int attempts = 0;
	mUserIdToVerificationData[userId] = { email, GenerateRandomCode(), attempts, now };
	mTimeQueue.push_back({ userId, now }); // Timestamp queue
}

bool Verify::RemoveUser(tUserId userId)
{
	if (!IsSubmitted(userId)) return false;

	RemoveUserFromQueue(userId);
	mUserIdToVerificationData.erase(userId); // Remove user from pending verification list;
	return true;
}

bool Verify::RemoveUserFromQueue(tUserId userId)
{
	for (auto it = mTimeQueue.begin(); it != mTimeQueue.end(); it++)
	{
		if (it->userId == userId)
		{
			mTimeQueue.erase(it);
			return true;
		}
	}
	return false;
}

Verify::SubmissionResults Verify::VerifyEmail(const std::string& email)
{
	if (email.empty() || email.size() > scMaxEmailLength)
		return SubmissionResults::EmailNameError;

	int atCount = 0;
	for (unsigned char c : email)
	{
		if (c == '@') atCount++;

		bool passed = std::isalnum(c) || c == '@' || c == '.' || c == '_' || c == '+' || c == '-';
		if (!passed) return SubmissionResults::EmailNameError; // Only allowed characters
	}
	if (atCount != 1)
		return SubmissionResults::EmailNameError; // Only one at symbol allowed

	size_t split = email.find('@');
	if (split == 0)
		return SubmissionResults::EmailNameError; // If email has no username

	std::string domain = email.substr(split + 1);
	domain = ToLower(domain);

	if (!IsValidDomain(domain))
		return SubmissionResults::EmailDomainError; // Wrong domain

	return SubmissionResults::Pass;
}

bool Verify::IsValidDomain(std::string_view domain) const
{
	for (auto validDomain : mcValidDomains)
	{
		if (domain == validDomain) return true;
	}
	return false;
}

std::string Verify::ToUpper(std::string str)
{
	for (char& c : str)
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	return str;
}

std::string Verify::ToLower(std::string str)
{
	for (char& c : str)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return str;
}


void Verify::TrimString(std::string& str)
{
	const char* ws = " \t\r\n";
	str.erase(0, str.find_first_not_of(ws));
	str.erase(str.find_last_not_of(ws) + 1);
}