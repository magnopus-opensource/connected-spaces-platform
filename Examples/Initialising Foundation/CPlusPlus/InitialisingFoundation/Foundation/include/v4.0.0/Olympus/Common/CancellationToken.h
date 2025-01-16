#pragma once

#include "Olympus/OlympusCommon.h"

namespace oly_common
{

class OLY_API CancellationToken
{
public:
    CancellationToken();
    OLY_NO_EXPORT CancellationToken(const CancellationToken& rhs) = delete;
    OLY_NO_EXPORT CancellationToken(const CancellationToken&& rhs) = delete;
    ~CancellationToken();

    CancellationToken& operator=(const CancellationToken& rhs) = delete;
    CancellationToken& operator=(const CancellationToken&& rhs) = delete;

    void Cancel();
    bool Cancelled();

    static CancellationToken& Dummy();

private:
    class Impl;
    Impl* ImplPtr;
};

} // namespace oly_common
