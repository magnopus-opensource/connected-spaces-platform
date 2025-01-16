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

namespace csp
{

constexpr unsigned int Info = 1;
constexpr unsigned int Warning = 2;
constexpr unsigned int Error = 3;
constexpr unsigned int Assert = 4;
constexpr unsigned int Network = 5;

} // namespace csp

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
#endif
    }

    /// @brief Writes the given Message to the log (both console and file streams).
    /// Categories can be the following values: 1 (InfoLog), 2 (WarningLog), 3 (ErrorLog), 4 (AssertLog), 5 (NetworkLog), 6 (GeneralLog)
    /// These categories are written to the streams to make it easier to find log types.
    /// @param File const char* : Name of the file to log
    /// @param Line const int : Line number to log
    /// @param Message const std::string : Message to log
    /// @param Category const int : Category of the log
    /// @param ShowLineNumber const bool : Whether to display the line number in the log
    static void LogOutput(const char* File, const int Line, const std::string Message, const int Category, const bool ShowLineNumber)
    {
        std::string CategoryStr = "";

        switch (Category)
        {
        case csp::Info:
            CategoryStr = "InfoLog";
            break;
        case csp::Warning:
            CategoryStr = "WarningLog";
            break;
        case csp::Error:
            CategoryStr = "ErrorLog";
            break;
        case csp::Assert:
            CategoryStr = "AssertLog";
            break;
        case csp::Network:
            CategoryStr = "NetworkLog";
            break;
        default:
            CategoryStr = "GeneralLog";
            break;
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

#define EXPAND(x) x
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME

#define CSP_LOG(Message) Logger::LogOutput(__FILE__, __LINE__, Message, csp::Info, false)

#define CSP_FORMATTED_LOG(Format, ...)                                                                                                               \
    {                                                                                                                                                \
        char CSPLogBuffer[256];                                                                                                                      \
        LoggerSprintf(CSPLogBuffer, 255, Format, ##__VA_ARGS__);                                                                                     \
        const std::string CSPLogFormattedMsg(CSPLogBuffer);                                                                                          \
        Logger::LogOutput(__FILE__, __LINE__, CSPDLogFormattedMsg, csp::Info, false);                                                                \
    }
