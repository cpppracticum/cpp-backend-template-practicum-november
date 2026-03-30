#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"

using namespace std::literals;
namespace net = boost::asio;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    std::cout << "=== RunWorkers: starting with " << n << " threads ===" << std::endl;
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
    std::cout << "=== RunWorkers: finished ===" << std::endl;
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        std::cout << "=== main: loading game ===" << std::endl;
        model::Game game = json_loader::LoadGame(argv[1]);

        std::cout << "Loaded " << game.GetMaps().size() << " maps" << std::endl;

        const unsigned num_threads = std::thread::hardware_concurrency();
        std::cout << "=== main: creating io_context with " << num_threads << " threads ===" << std::endl;
        net::io_context ioc(num_threads);

        std::cout << "=== main: setting up signal handlers ===" << std::endl;
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number) {
            if (!ec) {
                std::cout << "\nSignal " << signal_number << " received, shutting down..." << std::endl;
                ioc.stop();
            }
        });

        std::cout << "=== main: creating request handler ===" << std::endl;
        http_handler::RequestHandler handler{game};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        
        std::cout << "=== main: calling ServeHttp on " << address << ":" << port << " ===" << std::endl;
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });
        std::cout << "=== main: ServeHttp returned ===" << std::endl;

        std::cout << "Server has started..."sv << std::endl;

        std::cout << "=== main: calling RunWorkers ===" << std::endl;
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            std::cout << "=== ioc.run() starting ===" << std::endl;
            ioc.run();
            std::cout << "=== ioc.run() finished ===" << std::endl;
        });
        std::cout << "=== main: RunWorkers returned ===" << std::endl;
        
        std::cout << "Server stopped" << std::endl;
        
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "=== main: exiting ===" << std::endl;
    return EXIT_SUCCESS;
}
