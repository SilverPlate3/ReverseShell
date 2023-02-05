#include "Client.hpp"
#include "CommandExecuter.hpp"


int main()
{
    try {
        boost::asio::io_service ioService;

        boost::asio::ip::tcp::resolver resolver(ioService);
        Client client(ioService);

        ioService.run();
    }
    catch (std::fstream::failure& e) {
        std::cerr << e.what() << "\n";
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
