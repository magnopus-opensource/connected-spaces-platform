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

static std::string LogFilePath = "";

// @brief Sends data to file and console streams.
class Logger
{
public:
    /// @brief Writes the provided string to a file.
    /// File exists within the Build/Logs folder in the following format:
    /// LOG_"%Y-%m-%d_%H-%M-%S.txt
    /// @param : LogMessage const std::string : String to write
    static void SaveLogToFile(const std::string logMessage)
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
        OutFileStream << "[" + CurrentTime + ":" + std::to_string(ChronoMilliseconds) + "] " + logMessage;
        OutFileStream.close();
#else
        // Suppress the unused parameter warning
        (void)logMessage;
#endif
    }

    /// @brief Writes the given Message to the log (both console and file streams).
    /// Log level is written to the streams to make it easier to find log types.
    /// @param File const char* : Name of the file to log
    /// @param Line int : Line number to log
    /// @param Message const std::string& : Message to log
    /// @param LogLevel csp::common::LogLevel : Verbosity of the log
    /// @param ShowLineNumber bool : Whether to display the line number in the log
    static void LogOutput(const char* file, int line, const std::string& message, csp::common::LogLevel logLevel, bool showLineNumber)
    {
        std::string categoryStr = "";

        switch (logLevel)
        {
        case csp::common::LogLevel::NoLogging:
        {
            categoryStr = "NoLogging";
            break;
        }
        case csp::common::LogLevel::Fatal:
        {
            categoryStr = "WarningLog";
            break;
        }
        case csp::common::LogLevel::Error:
        {
            categoryStr = "WarningLog";
            break;
        }
        case csp::common::LogLevel::Warning:
        {
            categoryStr = "Warning";
            break;
        }
        case csp::common::LogLevel::Display:
        {
            categoryStr = "Display";
            break;
        }
        case csp::common::LogLevel::Log:
        {
            categoryStr = "Log";
            break;
        }
        case csp::common::LogLevel::Verbose:
        {
            categoryStr = "Verbose";
            break;
        }
        case csp::common::LogLevel::VeryVerbose:
        {
            categoryStr = "VeryVerbose";
            break;
        }
        default:
        case csp::common::LogLevel::All:
        {
            categoryStr = "All";
            break;
        }
        };

        std::string fileName(file);
        std::string outputMessage
            = showLineNumber ? categoryStr + ": " + fileName + "(" + std::to_string(line) + "): " + message : categoryStr + ": " + message;

        if (outputMessage.back() != '\n')
        {
            outputMessage += '\n';
        }

#ifdef CSP_WINDOWS
        // Print to VS console (if attached)
        OutputDebugStringA(outputMessage.c_str());
#endif

#ifdef CSP_ANDROID
        __android_log_print(ANDROID_LOG_INFO, CategoryStr.c_str(), "%s", OutputMessage.c_str());
#endif

        SaveLogToFile(outputMessage);
    }
};

#define CSP_LOG(Level, Message) Logger::LogOutput(__FILE__, __LINE__, Message, Level, false)
