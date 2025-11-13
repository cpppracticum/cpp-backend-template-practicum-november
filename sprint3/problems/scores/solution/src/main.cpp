#include "sdk.h"
#include "ticker.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "application.h"

using namespace std::literals;
namespace net = boost::asio;
namespace po = boost::program_options;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

struct Config {
    std::string config_file;
    std::string www_root;
    std::optional<int> tick_period;
    bool randomize_spawn_points = false;
};

std::optional<Config> ParseCommandLine(int argc, const char* argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<int>(), "set tick period")
        ("config-file,c", po::value<std::string>()->required(), "set config file path")
        ("www-root,w", po::value<std::string>()->required(), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions")
    ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        
        if (vm.count("help")) {
            std::cout << desc << "\n";
            return std::nullopt;
        }
        
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return std::nullopt;
    }
    
    Config config;
    config.config_file = vm["config-file"].as<std::string>();
    config.www_root = vm["www-root"].as<std::string>();
    
    if (vm.count("tick-period")) {
        config.tick_period = vm["tick-period"].as<int>();
    }
    
    if (vm.count("randomize-spawn-points")) {
        config.randomize_spawn_points = true;
    }
    
    return config;
}

}  // namespace

int main(int argc, const char* argv[]) {
    auto config_opt = ParseCommandLine(argc, argv);
    if (!config_opt) {
        return EXIT_FAILURE;
    }
    
    auto config = *config_opt;
    
    try {
        model::Game game = json_loader::LoadGame(config.config_file);
        app::Application application{game, config.randomize_spawn_points};
        
        const unsigned num_threads = std::max(1u, std::thread::hardware_concurrency());
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number) {
            if (!ec) {
                std::cout << "Signal " << signal_number << " received, shutting down..." << std::endl;
                ioc.stop();
            }
        });

        auto api_strand = net::make_strand(ioc);
        http_handler::RequestHandler handler{application, api_strand, config.tick_period.has_value()};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;
        
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });
        
        std::shared_ptr<Ticker> ticker;
        if (config.tick_period) {
            auto period = std::chrono::milliseconds(*config.tick_period);
            ticker = std::make_shared<Ticker>(api_strand, period,
                [&application](std::chrono::milliseconds delta) {
                    application.Tick(delta);
                }
            );
            ticker->Start();
            std::cout << "Auto-tick mode enabled with period: " << *config.tick_period << "ms" << std::endl;
        } else {
            std::cout << "Manual tick mode enabled (use /api/v1/game/tick)" << std::endl;
        }
        
        if (config.randomize_spawn_points) {
            std::cout << "Random spawn points enabled" << std::endl;
        } else {
            std::cout << "Fixed spawn points enabled" << std::endl;
        }

        std::cout << "Server has started..."sv << std::endl;
        std::cout << "Config file: " << config.config_file << std::endl;
        std::cout << "WWW root: " << config.www_root << std::endl;
        std::cout << "Listening on " << address << ":" << port << std::endl;
        std::cout << "Using " << num_threads << " threads" << std::endl;
        
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        
        if (ticker) {
            ticker->Stop();
        }
        
        std::cout << "Server shutdown complete" << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}