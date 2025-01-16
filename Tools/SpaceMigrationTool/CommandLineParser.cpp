#include "CommandLineParser.h"

#include "cxxopts.hpp"

#include <iostream>

const char* DefaultEndpointBaseURI = "https://ogs-odev.magnoboard.com";
const char* DefaultTenant = "OKO_TESTS";

CommandLineParser::CommandLineParser()
    : IsListSpaceOperation(false)
    , IsMigrateSpaceOperation(false)
    , IsShowHelpOperation(false)
{
}

void CommandLineParser::ParseCommandLine(int argc, const char* argv[])
{
    try
    {
        cxxopts::Options options(argv[0], "Space Migration tool");
        options.add_options()("u,User", "Email address for logging in to CHS", cxxopts::value<std::string>()->default_value("InvalidUser"))(
            "p,Password", "Password of the email address for logging in to CHS", cxxopts::value<std::string>()->default_value("InvalidPassword"))(
            "e,Endpoint", "CHS Endpoint where the requested operation will be ran",
            cxxopts::value<std::string>()->default_value(DefaultEndpointBaseURI))("t,Tenant", "CHS Tenant where the requested operation will be ran",
            cxxopts::value<std::string>()->default_value(DefaultTenant))("l,ListSpaces", "Show all spaces for the specified user")(
            "m,MigrateSpace", "Migrate space with the provided ID", cxxopts::value<std::string>()->default_value(""))("h,Help", "Print usage");

        auto result = options.parse(argc, argv);

        UserEmailAddress = result["User"].as<std::string>();
        UserPassword = result["Password"].as<std::string>();
        EndpointBaseURI = result["Endpoint"].as<std::string>();
        Tenant = result["Tenant"].as<std::string>();

        if (result.count("ListSpaces"))
        {
            IsListSpaceOperation = true;
        }
        else if (result.count("MigrateSpace"))
        {
            IsMigrateSpaceOperation = true;
            SpaceId = result["MigrateSpace"].as<std::string>();
        }
        else if (result.count("Help"))
        {
            IsShowHelpOperation = true;
            std::cout << options.help() << std::endl;
        }
    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "Error parsing command line options: " << e.what() << std::endl;
    }
}
