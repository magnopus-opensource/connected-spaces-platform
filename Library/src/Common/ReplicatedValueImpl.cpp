#include "ReplicatedValueImpl.h"

namespace csp::multiplayer
{
ReplicatedValueImpl::ReplicatedValueImpl(const ReplicatedValueImpl& Other) { Value = Other.Value; }

ReplicatedValueImpl& ReplicatedValueImpl::operator=(const ReplicatedValueImpl& Other)
{
    std::visit([this](auto&& Rhs) { Value = Rhs; }, Other.Value);
    return *this;
}

bool ReplicatedValueImpl::operator==(const ReplicatedValueImpl& Other) const
{
    bool Result = false;

    std::visit(
        [this, &Result, &Other](auto&& Lhs)
        {
            using T = std::decay_t<decltype(Lhs)>;
            Result = (std::get<T>(Value) == std::get<T>(Other.Value));
        },
        Value);

    return Result;
}

bool ReplicatedValueImpl::operator!=(const ReplicatedValueImpl& Other) const { return !(*this == Other); }

bool ReplicatedValueImpl::operator<(const ReplicatedValueImpl& Other) const
{
    bool Result = false;

    std::visit(
        [this, &Result, &Other](auto&& Lhs)
        {
            std::visit(
                [this, &Result, Other](auto&& Rhs)
                {
                    using LT = std::decay_t<decltype(Lhs)>;
                    using RT = std::decay_t<decltype(Rhs)>;

                    if constexpr (std::is_integral_v<LT> && std::is_integral_v<RT>)
                    {
                        Result = (std::get<LT>(Value) < std::get<RT>(Other.Value));
                    }
                },
                Other.Value);
        },
        Value);

    return Result;
}

bool ReplicatedValueImpl::operator>(const ReplicatedValueImpl& Other) const
{
    bool Result = false;

    std::visit(
        [this, &Result, &Other](auto&& Lhs)
        {
            std::visit(
                [this, &Result, Other](auto&& Rhs)
                {
                    using LT = std::decay_t<decltype(Lhs)>;
                    using RT = std::decay_t<decltype(Rhs)>;

                    if constexpr (std::is_integral_v<LT> && std::is_integral_v<RT>)
                    {
                        Result = (std::get<LT>(Value) > std::get<RT>(Other.Value));
                    }
                },
                Other.Value);
        },
        Value);

    return Result;
}

}
