#include <cstdlib>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <vector>

#include "config.hpp"
#include "serial.hpp"
#include "udp.hpp"

namespace
{
using namespace multicast;
void print_usage(std::string_view basename)
{
    std::cout << "Usage: " << basename << " <CONFIG_FILE>\n";
}

void print_prompt()
{
    std::cout << "Enter message: ";
}

struct Client
{
    virtual ~Client() = default;
    virtual bool send_message(std::string_view) = 0;
};

class UdpClient : public Client
{
public:
    UdpClient(const udp::Socket& socket, const sockaddr_in& address)
        : socket_(socket), address_(address)
    {
    }

    bool send_message(std::string_view message) final
    {
        return udp::send(socket_, address_, message);
    }

private:
    const udp::Socket& socket_;
    sockaddr_in address_;
};

class SerialClient : public Client
{
public:
    explicit SerialClient(std::string_view path) : device_(path)
    {
    }

    bool send_message(std::string_view message) final
    {
        return serial::send(device_, message);
    }

private:
    serial::Device device_;
};

} // namespace

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string_view(argv[1]) == "-h")
    {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (argc < 2)
    {
        std::cerr << "Not enough arguments!\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        Config config{argv[1]};
        udp::Socket socket{};
        std::vector<std::unique_ptr<Client>> clients;
        for (const auto& entry : config.GetUdpEntries())
        {
            clients.push_back(
                std::make_unique<UdpClient>(socket, entry.address));
        }

        for (const auto& entry : config.GetSerialEntries())
        {
            clients.push_back(std::make_unique<SerialClient>(entry.device));
        }

        std::string input{};
        while (print_prompt(), std::getline(std::cin, input))
        {
            input.push_back('\n');
            for (const auto& client : clients)
            {
                client->send_message(input);
            }
        }
        return EXIT_SUCCESS;
    }
    catch (const std::exception& err)
    {
        std::cerr << "Multicast failure: " << err.what() << '\n';
    }
    return EXIT_FAILURE;
}
