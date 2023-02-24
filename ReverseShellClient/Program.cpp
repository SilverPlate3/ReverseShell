#include "Client.h"
#include "NetworkExcetion.hpp"
#include "WinApiUtils.hpp"
#include <boost/process/error.hpp>

void Start()
{
	std::string connectionMessage("First connection made. No previous exception forced a new connection");
	while (true)
	{
		try
		{
			Client client(connectionMessage);
		}
		catch (const std::fstream::failure& ex) { connectionMessage = ex.what(); }
		catch (const boost::process::process_error& ex) { connectionMessage = ex.what(); }
		catch (NetworkExcetion& ex) { connectionMessage = ex.what(); }
		catch (WinApiException& ex) { connectionMessage = ex.what(); }
	}
}

int main()
{
    try {
        Start();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
