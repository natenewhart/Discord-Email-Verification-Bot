// Nate Newhart - EnvLoader.h

#pragma once

#include <fstream>
#include <string>
#include <cstdlib>
#include <stdexcept>

namespace Env
{
	inline bool load(const std::string& filename = ".env"); // Load environment variables from a .env file
	inline std::string getVar(const std::string& name, const std::string& fallback);
	inline bool hasVar(const char* name); // Check if an environment variable exists

	namespace detail
	{
		inline bool tryGetVar(const char* name, std::string& out); // If an environment variable exists, return its value in 'out' and return true
		inline int setVar(const char* name, const char* value); // Platform-specific environment variable setter
		inline void trimString(std::string& str); // Trim leading and trailing whitespace of string
	}
}

// ------------------- Environment Function Definitions -------------------

inline bool Env::load(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
		return false;

	std::string line;
	while (std::getline(file, line))
	{
		detail::trimString(line); // Trim leading and trailing whitespace

		if (line.empty() || line[0] == '#') // Skip empty lines and comments
			continue;

		size_t pos = line.find('=');
		if (pos != std::string::npos)
		{
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);

			detail::trimString(key);
			detail::trimString(value);
			if (key.empty()) continue; // Skip empty keys

			detail::setVar(key.c_str(), value.c_str());
		}
	}
	file.close();
	return true;
}

inline std::string Env::getVar(const std::string& name, const std::string& fallback)
{
	std::string value;
	if (!detail::tryGetVar(name.c_str(), value))
	{
		return fallback;
	}
	return value;
}

inline bool Env::hasVar(const char* name)
{
	std::string _;
	return detail::tryGetVar(name, _);
}

// ------------------- Detail Functions -------------------

inline bool Env::detail::tryGetVar(const char* name, std::string& out)
{
#if defined(_WIN32)
	char* buf = nullptr;
	size_t len = 0;
	if (_dupenv_s(&buf, &len, name) != 0 || buf == nullptr)
	{
		std::free(buf);
		return false;
	}
	out.assign(buf);
	std::free(buf);
	return true;
#else
	const char* v = std::getenv(name);
	if (!v)
		return false;
	out.assign(v);
	return true;
#endif
}

inline int Env::detail::setVar(const char* name, const char* value)
{
	if (!name || !value)
		return -1;
	if (hasVar(name))
		return -1; // Do not overwrite existing environment variables
	
	#if defined(_WIN32)
		return _putenv_s(name, value);
	#else
		return setenv(name, value, 1);
	#endif
}

inline void Env::detail::trimString(std::string& str)
{
	const char* ws = " \t\r\n";
	str.erase(0, str.find_first_not_of(ws));
	str.erase(str.find_last_not_of(ws) + 1);
}