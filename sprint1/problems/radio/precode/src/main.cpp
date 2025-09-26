#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::literals;
namespace net = boost::asio;
using net::ip::udp;

void StartServer(uint16_t port) {
    try {
        net::io_context io_context;
        
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
        
        std::cout << "UDP Server listening on port " << port << std::endl;

        while (true) {
            std::vector<char> buffer(65507);
            udp::endpoint remote_endpoint;

            size_t length = socket.receive_from(net::buffer(buffer), remote_endpoint);
            
            std::cout << "Received " << length << " bytes from " 
                      << remote_endpoint.address().to_string() << std::endl;

            Player player(ma_format_u8, 1);
            int frame_size = player.GetFrameSize();
            size_t frames = length / frame_size;

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                1.5s * frames / 65000
            );
            
            if (frames > 0) {
                player.PlayBuffer(buffer.data(), frames, duration);
                std::cout << "Played " << frames << " frames (" 
                          << duration.count() << " ms)" << std::endl;
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Server exception: " << e.what() << std::endl;
    }
}

void StartClient(uint16_t port) {
    try {
        net::io_context io_context;
        udp::socket socket(io_context, udp::v4());
        
        Recorder recorder(ma_format_u8, 1);

        while (true) {
            std::string server_ip;
            std::cout << "Enter server IP: ";
            std::getline(std::cin, server_ip);

            if (server_ip.empty()) {
                continue;
            }

            auto rec_result = recorder.Record(65000, 1.5s);
            int frame_size = recorder.GetFrameSize();
            size_t data_size = rec_result.frames * frame_size;

            udp::endpoint endpoint(
                net::ip::make_address(server_ip), 
                port
            );
            
            socket.send_to(
                net::buffer(rec_result.data.data(), data_size), 
                endpoint
            );
            
            std::cout << "Sent " << data_size << " bytes (" << rec_result.frames 
                      << " frames) to " << server_ip << std::endl;
        }
    } catch (std::exception& e) {
        std::cerr << "Client exception: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <client|server> <port>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    uint16_t port = std::stoi(argv[2]);

    if (mode == "server") {
        StartServer(port);
    } else if (mode == "client") {
        StartClient(port);
    } else {
        std::cerr << "Invalid mode. Use 'client' or 'server'" << std::endl;
        return 1;
    }

    return 0;
}
