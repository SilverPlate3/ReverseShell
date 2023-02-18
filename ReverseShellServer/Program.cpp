#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Server.h"
#include "NetworkExcetion.hpp"

void Start()
{
    while (true)
    {
        try {
            Server server;
        }
        catch (const std::fstream::failure& ex) { std::cerr << ex.what() << std::endl;}
        catch (NetworkExcetion& ex) { std::cerr << ex.what() << std::endl;}
    }
}

void main()
{
    try{
        Start();
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }
}