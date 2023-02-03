#include "Client.hpp"
#include "CommandExecuter.hpp"


int main()
{
    SystemCommand::CommandExecuter commandExecuter;
    std::unique_ptr<std::string> results = std::move(commandExecuter.RunCommand(R"(whoami)"));
    std::cout << *results << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    std::unique_ptr<std::string> results2 = std::move(commandExecuter.RunCommand(R"(ping -n 20 9.8.8.8.8)"));
    std::cout << *results2 << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    std::unique_ptr<std::string> results3 = std::move(commandExecuter.RunCommand(R"(dir)"));
    std::cout << *results3 << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    std::unique_ptr<std::string> results4 = std::move(commandExecuter.RunCommand(R"(cd C:\Users\aridels)"));
    std::cout << *results4 << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    std::unique_ptr<std::string> results5 = std::move(commandExecuter.RunCommand(R"(sdfdss)"));
    std::cout << *results5 << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    /*try {
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
    }*/

    return 0;
}
