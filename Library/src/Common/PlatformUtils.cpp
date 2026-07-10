#include "PlatformUtils.h"

namespace csp::common
{

std::optional<std::string> GetEnvironmentVariableValue(const std::string& Name)
{
#if defined(_MSC_VER)
    // Prevent C2220 MSVC CRT warning
    char* Value = nullptr;
    size_t ValueLength = 0;

    const errno_t Result = _dupenv_s(&Value, &ValueLength, Name.c_str());

    if (Result != 0 || Value == nullptr)
    {
        return std::nullopt;
    }

    std::string ResultValue { Value };
    free(Value);

    if (ResultValue.empty())
    {
        return std::nullopt;
    }

    return ResultValue;
#else
    const char* Value = std::getenv(Name.c_str());

    if (Value == nullptr)
    {
        return std::nullopt;
    }

    return std::string { Value };
#endif
}
}
