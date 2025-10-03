/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "CSP/Common/Systems/Log/LogLevels.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#ifdef CSP_WINDOWS
#include <Windows.h> // for printing to VS console
#include <filesystem>
#define LoggerSprintf _snprintf_s
#else
#define LoggerSprintf snprintf
#endif

#ifdef CSP_ANDROID
#include <android/log.h>
#endif

typedef std::chrono::system_clock Clock;

#define RESET "\033[0m"

static std::string LogFilePath = "";

// @brief Sends data to file and console streams.
class Logger
{
public:
    /// @brief Writes the provided string to a file.
    /// File exists within the Build/Logs folder in the following format:
    /// LOG_"%Y-%m-%d_%H-%M-%S.txt
    /// @param : LogMessage const std::string : String to write
    static void SaveLogToFile(const std::string LogMessage)
    {
#if (defined DEBUG) && (defined _WINDOWS)
        time_t Rawtime;
        struct tm TimeInfo;
        char Buffer[80];

        time(&Rawtime);
        localtime_s(&TimeInfo, &Rawtime);

        strftime(Buffer, sizeof(Buffer), "%Y-%m-%d_%H-%M-%S", &TimeInfo);
        const std::string CurrentTime(Buffer);

        if (LogFilePath == "")
        {
            std::filesystem::create_directory(".\\Logs");
            LogFilePath = ".\\Logs\\Log_" + CurrentTime + ".txt";
        }

        auto ChronoNow = Clock::now();
        const std::chrono::duration<double> TimeSinceEpoch = ChronoNow.time_since_epoch();
        std::chrono::seconds::rep ChronoMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(TimeSinceEpoch).count() % 1000;

        std::ofstream OutFileStream;
        OutFileStream.open(LogFilePath, std::ios_base::app);
        OutFileStream << "[" + CurrentTime + ":" + std::to_string(ChronoMilliseconds) + "] " + LogMessage;
        OutFileStream.close();
#else
        // Suppress the unused parameter warning
        (void)LogMessage;
#endif
    }

    /// @brief Writes the given Message to the log (both console and file streams).
    /// Log level is written to the streams to make it easier to find log types.
    /// @param File const char* : Name of the file to log
    /// @param Line int : Line number to log
    /// @param Message const std::string& : Message to log
    /// @param LogLevel csp::common::LogLevel : Verbosity of the log
    /// @param ShowLineNumber bool : Whether to display the line number in the log
    static void LogOutput(const char* File, int Line, const std::string& Message, csp::common::LogLevel LogLevel, bool ShowLineNumber)
    {
        std::string CategoryStr = "";

        switch (LogLevel)
        {
        case csp::common::LogLevel::NoLogging:
        {
            CategoryStr = "NoLogging";
            break;
        }
        case csp::common::LogLevel::Fatal:
        {
            CategoryStr = "WarningLog";
            break;
        }
        case csp::common::LogLevel::Error:
        {
            CategoryStr = "WarningLog";
            break;
        }
        case csp::common::LogLevel::Warning:
        {
            CategoryStr = "Warning";
            break;
        }
        case csp::common::LogLevel::Display:
        {
            CategoryStr = "Display";
            break;
        }
        case csp::common::LogLevel::Log:
        {
            CategoryStr = "Log";
            break;
        }
        case csp::common::LogLevel::Verbose:
        {
            CategoryStr = "Verbose";
            break;
        }
        case csp::common::LogLevel::VeryVerbose:
        {
            CategoryStr = "VeryVerbose";
            break;
        }
        default:
        case csp::common::LogLevel::All:
        {
            CategoryStr = "All";
            break;
        }
        };

        std::string FileName(File);
        std::string OutputMessage
            = ShowLineNumber ? CategoryStr + ": " + FileName + "(" + std::to_string(Line) + "): " + Message : CategoryStr + ": " + Message;

        if (OutputMessage.back() != '\n')
        {
            OutputMessage += '\n';
        }

#ifdef CSP_WINDOWS
        // Print to VS console (if attached)
        OutputDebugStringA(OutputMessage.c_str());
#endif

#ifdef CSP_ANDROID
        __android_log_print(ANDROID_LOG_INFO, CategoryStr.c_str(), "%s", OutputMessage.c_str());
#endif

        SaveLogToFile(OutputMessage);
    }
};

#define CSP_LOG(Level, Message) Logger::LogOutput(__FILE__, __LINE__, Message, Level, false)
