#pragma once

#include <iostream>
#include <boost/process/io.hpp>
#include <boost/process/system.hpp>

class CommandExecuter
{
public:
	static const int ShellCommandErrorCode = 1999;
private:
	static std::string m_Binary;
	boost::process::ipstream m_stdOut;
	boost::process::ipstream m_stdErr;

public:
	std::string RunShellCommand(const std::string& shellCommand);
	const boost::process::child CreateChildProcess(const std::string& shellCommand);
	std::string GetCommandResults(const boost::process::child& childProcess);
};