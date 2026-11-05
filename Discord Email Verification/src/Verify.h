// Nate Newhart - Verify.h

// Email verification object; stores user ids, timestamps, and verification requests
// Submit user id and then confirm them with the inputted code

#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <queue>

#include <CRandom/CRandom.hpp>

class Verify
{
public:
	enum class SubmissionResults // Result types for SubmitUser
	{
		CooldownFailure, Pass
	};
	enum class ConfirmationResults // Result types for ConfirmUser
	{
		Pass, UserNotFound, WrongCode, TooManyAttempts,
	};
private:
	using tUserId = uint64_t; // Integer type wrapper for user IDs
	struct UserData
	{
		//std::string email; // TODO: is this needed
		std::string verificationCode;
		int confirmationAttempts = 0;
		std::chrono::steady_clock::time_point timestamp; // timestamp for submission cooldown
	};
	struct QueueEntry
	{
		tUserId userId;
		std::chrono::steady_clock::time_point timestamp;
	};

public:
	Verify() = default;

	SubmissionResults SubmitUser(tUserId userId); // Store user data and generate code for verification return true if user is not in system. If user is already in system return false and fail
	ConfirmationResults ConfirmUser(tUserId userId, const std::string& code); // Check if the provided code is correct and return true if passed
	std::string GetVerificationCode(tUserId userId); // Get the verification code tied to userId
	bool IsSubmitted(tUserId userId); // Has user been submitted but not confirmed

	void RemoveExpiredVerifications(); // Remove expired verification codes from the system

private:
	std::string GenerateRandomCode();

	void AddUser(tUserId userId);    // Overwrite and add user
	bool RemoveUser(tUserId userId); // Remove user return true if user is removed
	bool RemoveUserFromQueue(tUserId userId); // Move item in queue to the back of the queue

	bool VerifyEmail();
	static std::string ToUpper(std::string str);

private:
	CRandom mRNG;
	std::unordered_map<tUserId, UserData> mUserIdToVerificationData; // Maps user IDs to their verification data
	std::deque<QueueEntry> mTimeQueue; // Queue to track the order of user submissions for verification

	// TODO: location of constexpr's maybe in nameesapce
	static constexpr int scCodeLength = 6; // Length of hexadecimal verification code (must be in range [1, 16])
	static constexpr int scMaxConfirmationAttempts = 5;
	static constexpr std::chrono::minutes scVerificationLifetime = std::chrono::minutes(3); // Lifetime of verification code
	static constexpr std::chrono::seconds scSubmissionCooldown   = std::chrono::seconds(20);
};