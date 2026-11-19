#include "Verify.h"
#include <format>
#include <print>
#include <random>

Verify::SubmissionResults Verify::SubmitUser(const std::vector<std::string>& validDomains, tUserId userId, std::string email, std::string& outCode)
{
	std::lock_guard<std::mutex> lock(mMutex);

	RemoveExpiredSubmissionsLocked();
	if (IsSubmittedLocked(userId))
	{
		auto now = std::chrono::steady_clock::now();
		if (now - mUserIdToVerificationData[userId].timestamp < scSubmissionCooldown) // Cooldown failure
			return SubmissionResults::CooldownFailure;
		
		RemoveUserLocked(userId); // Overwrite user when they double submit
	}

	// TODO: email cooldown doesnt apply if verification doesnt work; cooldown is only applied if you successfully submitted
	TrimString(email);
	auto emailResults = VerifyEmail(validDomains, email);
	if (emailResults != SubmissionResults::Pass) return emailResults; // Return error value from email verification

	AddUserLocked(userId, email);
	outCode = mUserIdToVerificationData[userId].verificationCode;
	return SubmissionResults::Pass;
}

Verify::ConfirmationResults Verify::ConfirmUser(tUserId userId, const std::string& code)
{
	std::lock_guard<std::mutex> lock(mMutex);

	RemoveExpiredSubmissionsLocked();
	if (!IsSubmittedLocked(userId)) return ConfirmationResults::UserNotFound; // User is not in system so fail

	std::string normCode = Verify::ToUpper(code);
	if (mUserIdToVerificationData[userId].verificationCode == normCode)
	{
		RemoveUserLocked(userId);
		return ConfirmationResults::Pass;
	}

	mUserIdToVerificationData[userId].confirmationAttempts++;
	if (mUserIdToVerificationData[userId].confirmationAttempts == scMaxConfirmationAttempts)
	{
		RemoveUserLocked(userId);
		return ConfirmationResults::TooManyAttempts;
	}

	return ConfirmationResults::WrongCode;
}

std::string Verify::GetVerificationCode(tUserId userId)
{
	// TODO: use at() or use [...].verificationCode? IF YOU SWITCH MAKE SURE TO USE MUTEX
	std::lock_guard<std::mutex> lock(mMutex);
	return mUserIdToVerificationData.at(userId).verificationCode; // Return the verification code for the user
}

bool Verify::IsSubmittedLocked(tUserId userId)
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

void Verify::RemoveExpiredSubmissionsLocked()
{
	const auto now = std::chrono::steady_clock::now();

	while (!mTimeQueue.empty() &&
		   now - mTimeQueue.front().timestamp > scVerificationLifetime)
	{
		mUserIdToVerificationData.erase(mTimeQueue.front().userId);
		mTimeQueue.pop_front();
	}
}

void Verify::AddUserLocked(tUserId userId, const std::string& email)
{
	auto code = GenerateRandomCode();
	auto now  = std::chrono::steady_clock::now();
	int attempts = 0;
	mUserIdToVerificationData[userId] = { email, code, attempts, now };
	mTimeQueue.push_back({ userId, now }); // Timestamp queue
}

bool Verify::RemoveUserLocked(tUserId userId)
{
	if (!IsSubmittedLocked(userId)) return false;

	RemoveUserFromQueue(userId);
	mUserIdToVerificationData.erase(userId); // Remove user from pending verification list;
	return true;
}

bool Verify::RemoveUser(tUserId userId)
{
	std::lock_guard<std::mutex> lock(mMutex);
	return RemoveUserLocked(userId);
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

Verify::SubmissionResults Verify::VerifyEmail(const std::vector<std::string>& validDomains, const std::string& email) const
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

	if (!IsValidDomain(validDomains, domain))
		return SubmissionResults::EmailDomainError; // Wrong domain

	return SubmissionResults::Pass;
}

bool Verify::IsValidDomain(const std::vector<std::string>& validDomains, std::string_view domain) const
{
	for (auto validDomain : validDomains)
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