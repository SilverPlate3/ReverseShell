#include "Client.h"
#include "NetworkExcetion.hpp"
#include <boost/process/error.hpp>

void Start()
{
    while (true)
    {
        try {
            Client client;
        }
        catch (const std::fstream::failure& ex) { std::cerr << ex.what() << std::endl; }
        catch (const boost::process::process_error& ex) { std::cerr << ex.what() << std::endl; }
        catch (NetworkExcetion& ex) { std::cerr << ex.what() << std::endl; }
    }
}

void main()
{
    try {
        Start();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
