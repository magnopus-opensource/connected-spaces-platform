// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "_exports.h"
#include <memory>
#include <functional>
#include "connection_state.h"
#include "trace_level.h"
#include "log_writer.h"
#include "signalr_client_config.h"
#include "transfer_format.h"

namespace signalr
{
    class connection_impl;

    class connection
    {
    public:
        typedef std::function<void __cdecl(const std::string&)> message_received_handler;

        SIGNALRCLIENT_API explicit connection(const std::string& url, trace_level trace_level = trace_level::info, std::shared_ptr<log_writer> log_writer = nullptr);

        SIGNALRCLIENT_API ~connection();

        connection(const connection&) = delete;

        connection& operator=(const connection&) = delete;

        SIGNALRCLIENT_API void __cdecl start(std::function<void(std::exception_ptr)> callback) noexcept;

        SIGNALRCLIENT_API void __cdecl send(const std::string& data, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept;

        SIGNALRCLIENT_API void __cdecl set_message_received(const message_received_handler& message_received_callback);
        SIGNALRCLIENT_API void __cdecl set_disconnected(const std::function<void __cdecl(std::exception_ptr)>& disconnected_callback);

        SIGNALRCLIENT_API void __cdecl set_client_config(signalr_client_config& config);

        SIGNALRCLIENT_API void __cdecl stop(std::function<void(std::exception_ptr)> callback, std::exception_ptr exception) noexcept;

        SIGNALRCLIENT_API connection_state __cdecl get_connection_state() const noexcept;
        SIGNALRCLIENT_API std::string __cdecl get_connection_id() const;

    private:
        // The recommended smart pointer to use when doing pImpl is the `std::unique_ptr`. However
        // we are capturing the m_pImpl instance in the lambdas used by tasks which can outlive
        // the connection instance. Using `std::shared_ptr` guarantees that we won't be using
        // a deleted object if the task is run after the `connection` instance goes away.
        std::shared_ptr<connection_impl> m_pImpl;
    };
}
