#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Server.hpp"

int main()
{
    try
    {
        Server server;
    }
    catch (std::exception& e) 
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}