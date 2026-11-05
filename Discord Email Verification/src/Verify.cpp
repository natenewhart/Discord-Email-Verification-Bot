#include "Verify.h"
#include <format>
#include <print>

Verify::SubmissionResults Verify::SubmitUser(tUserId userId)
{
	RemoveExpiredVerifications();
	if (IsSubmitted(userId))
	{
		auto now = std::chrono::steady_clock::now();
		if (now - mUserIdToVerificationData[userId].timestamp < scSubmissionCooldown) // Cooldown failure
			return SubmissionResults::CooldownFailure;
		
		RemoveUser(userId); // Overwrite user when they double submit
	}

	// TODO: Implement email check and return the reason why verify doesnt work

	AddUser(userId);
	return SubmissionResults::Pass;
}

Verify::ConfirmationResults Verify::ConfirmUser(tUserId userId, const std::string& code)
{
	RemoveExpiredVerifications();
	if (!IsSubmitted(userId)) return ConfirmationResults::UserNotFound; // User is not in system so fail

	if (mUserIdToVerificationData[userId].verificationCode == code)
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

std::string Verify::GetVerificationCode(tUserId userId)
{
	// TODO: make this safe from calling when user is not in system
	// uses .at() so code fails

	return mUserIdToVerificationData.at(userId).verificationCode; // Return the verification code for the user
}

bool Verify::IsSubmitted(tUserId userId)
{
	return mUserIdToVerificationData.find(userId) != mUserIdToVerificationData.end();
}

// Private Helpers

std::string Verify::GenerateRandomCode()
{
	static_assert(scCodeLength >= 1 && scCodeLength <= 15, "code length out of range"); // Code length must match bounds
	uint64_t max = (1ULL << (4 * scCodeLength)) - 1; // Power of 16 optimization; 16^n where n is digits
	uint64_t code = mRNG.uInt64(0, max);
	return std::format("{:0{}X}", code, scCodeLength); // Convert to hexadecimal string
}

void Verify::RemoveExpiredVerifications()
{
	const auto now = std::chrono::steady_clock::now();

	while (!mTimeQueue.empty() &&
		   now - mTimeQueue.front().timestamp > scVerificationLifetime)
	{
		mUserIdToVerificationData.erase(mTimeQueue.front().userId);
		mTimeQueue.pop_front();
	}
}

void Verify::AddUser(tUserId userId)
{
	auto now = std::chrono::steady_clock::now();
	mUserIdToVerificationData[userId] = { GenerateRandomCode(), 0, now };
	mTimeQueue.push_back({ userId, now });
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
