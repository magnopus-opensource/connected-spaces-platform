#pragma once
#include <string>

/*
 * RAII container object to facilitate automatically logging out when leaving scope
 * Logs in upon construction, Logs out upon destruction.
 */
class LoginRAII
{
public:
	LoginRAII(const std::string& AccountLoginEmail, const std::string& AccountPassword);
	~LoginRAII();
};