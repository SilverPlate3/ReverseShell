#include <boost/process.hpp>
#include <boost/format.hpp>
#include <boost/process/extend.hpp>
#include <filesystem>
#include <sstream>
#include "CommandExecuter.h"

std::string CommandExecuter::m_Binary(R"(C:\Windows\System32\WrongPath\v1.0\powershell.exe)");

std::string CommandExecuter::RunShellCommand(const std::string& shellCommand)
{
	auto childProcess = CreateChildProcess(shellCommand);
	std::cout << "PID: " << childProcess.id() << std::endl;

	bool processExited = childProcess.wait_for(std::chrono::seconds(10));
	if (!processExited)
		return std::string("Command didn't exit in 10 seconds! Didn't wait for it to finish...");

	return std::move(GetCommandResults(childProcess));
}


const boost::process::child CommandExecuter::CreateChildProcess(const std::string& shellCommand)
{
	namespace bp = boost::process;
	namespace ex = bp::extend;

	auto DoesBinaryExist = [](auto& exec)
	{
		std::string processPath(exec.exe);
		std::string ProcessName(std::filesystem::path(processPath).filename().string());

		if (std::filesystem::exists(processPath))
		{
			std::cout << processPath << "  exists!" << std::endl;
			return;
		}

		std::string newPath(bp::search_path(ProcessName).string());
		if (newPath.empty())
		{
			std::error_code ec{ ShellCommandErrorCode, std::system_category() };
			exec.set_error(ec, "Can't run command as boost::process doesn't find " + ProcessName);
		}

		m_Binary = newPath;
		exec.exe = m_Binary.c_str();
	};

	auto OnError = [](auto& exec, const std::error_code&)
	{
		std::error_code ec{ ShellCommandErrorCode, std::system_category() };
		exec.set_error(ec, "An error occured while running the command");
	};

	auto childProcess = bp::child(
		m_Binary,
		shellCommand,
		bp::std_out > m_stdOut,
		bp::std_err > m_stdErr,
		ex::on_setup = DoesBinaryExist,
		ex::on_error = OnError
	);

	return std::move(childProcess);
}


std::string CommandExecuter::GetCommandResults(const boost::process::child& childProcess)
{
	std::stringstream output, error;
	m_stdOut >> output.rdbuf();
	m_stdErr >> error.rdbuf();
	m_stdOut.close();
	m_stdErr.close();
	auto result = (boost::format("\nExit code: %1% \nStdOut: %2% \nstdErr: %3%") % childProcess.exit_code() % output.str() % error.str()).str();
	return std::move(result);
}
