#pragma once
#include <string>

class CommandLineParser
{
public:
    CommandLineParser();
    void ParseCommandLine(int argc, const char* argv[]);

    std::string UserEmailAddress;
    std::string UserPassword;
    std::string EndpointBaseURI;
    std::string Tenant;
    std::string SpaceId;
    bool IsListSpaceOperation;
    bool IsMigrateSpaceOperation;
    bool IsShowHelpOperation;
};
