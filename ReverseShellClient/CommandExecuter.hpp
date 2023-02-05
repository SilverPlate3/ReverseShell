#pragma once

#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>
#include <sstream>
#include <vector>
#include <memory>


class CommandResult
{
private:
    std::string m_output;
    int m_exitCode;

    friend class CommandExecuter;

    CommandResult() = default;

    CommandResult(const std::string output, const int exitCode)
        : m_output(output), m_exitCode(exitCode) {}

    CommandResult& operator=(CommandResult&& other) noexcept
    {
        if (this != &other)
        {
            m_output = other.m_output;
            m_exitCode = other.m_exitCode;
        }
        return *this;
    }

    std::unique_ptr<std::string> ConcatenateResults()
    {
        return std::make_unique<std::string>("ExitCode: " + std::to_string(m_exitCode) + "\nCommandOutput: " + m_output);
    }
};

class CommandExecuter
{
private:
    int maxSecondsToWaitForCommand = 10;
    std::mutex mutexCommandResult;
    bool commandCompleted = false;
    CommandResult commandResults;

public:

    std::unique_ptr<std::string> RunCommand(const std::string& command)
    {
        std::thread t(&CommandExecuter::popen, this, command);
        if (t.joinable())
            t.detach();

        WaitForCommandOutput();

        std::unique_lock<std::mutex> lock(mutexCommandResult);
        if (!commandCompleted)
        {
            std::stringstream ss;
            ss << "Command took more then " << maxSecondsToWaitForCommand << " Seconds."
                << "The command still executed, but we didn't wait for its output.";

            commandResults = CommandResult(ss.str(), -1);
        }

        commandCompleted = false;
        return commandResults.ConcatenateResults();
    }

private:

    void popen(const std::string command)
    {
        auto pipe = _popen(command.c_str(), "r");
        if (pipe == nullptr)
        {
            commandResults = CommandResult("popen failed! command didn't run!", -1);
            commandCompleted = true;
            return;
        }

        std::fstream fstream(pipe);
        std::vector<char> output((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
        std::unique_lock<std::mutex> lock(mutexCommandResult, std::try_to_lock);
        if (!lock.owns_lock())
            return;

        auto exitcode = _pclose(pipe);
        FillEmptyCommandOutput(output);
        output.data()[output.size() - 1] = '\0';

        commandResults = CommandResult(output.data(), exitcode);
        commandCompleted = true;
    }

    void WaitForCommandOutput()
    {
        using Timer = std::chrono::steady_clock;
        auto start = Timer::now();
        auto end = start + std::chrono::seconds(maxSecondsToWaitForCommand);
        while (!commandCompleted && (Timer::now() < end))
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void FillEmptyCommandOutput(std::vector<char>& output)
    {
        if (output.empty())
        {
            std::string fallBackOutput("Command output is empty ");
            std::for_each(fallBackOutput.begin(), fallBackOutput.end(), [&](const char c) {output.emplace_back(c); });
        }
    }
};
