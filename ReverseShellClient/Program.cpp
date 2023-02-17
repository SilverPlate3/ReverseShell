#include "Client.hpp"


int main()
{
    try {
        Client client;
    }
    catch (std::fstream::failure& e) {
        std::cerr << e.what() << "\n";
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
