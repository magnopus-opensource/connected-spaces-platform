#pragma once

#include <cstdlib>
#include <optional>
#include <string>

#if defined(_MSC_VER)
#include <stdlib.h>
#endif

namespace csp::common
{

std::optional<std::string> GetEnvironmentVariableValue(const std::string& Name);

}
