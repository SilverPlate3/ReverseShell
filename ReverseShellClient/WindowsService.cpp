#include "WindowsService.h"


void WindowsServices::CreateWindowsService()
{
	auto serviceControllerHandle(OpenSercivesManager());
	WinApiUtils winApiUtils;
	const std::string serviceName = "ArielSilver99";

	auto servicehandle = CreateServiceA(
		serviceControllerHandle,
		serviceName.c_str(),
		serviceName.c_str(),
		SC_MANAGER_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DISABLED, // TODO - change to AutoStart
		SERVICE_ERROR_IGNORE,
		winApiUtils.GetCurrentPrcessPathChar(),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!servicehandle)
		throw WinApiException("CreateServiceA");
}


SC_HANDLE WindowsServices::OpenSercivesManager()
{
	SC_HANDLE serviceControllerHandle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!serviceControllerHandle)
		throw WinApiException("OpenSCManagerA");

	return serviceControllerHandle;
}








































