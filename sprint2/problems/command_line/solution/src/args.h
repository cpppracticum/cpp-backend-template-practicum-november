#pragma once

#include <boost/program_options.hpp>
#include <iostream>
#include <optional>
#include <string>

namespace po = boost::program_options;

struct ServerArgs {
    std::string config_file;
    std::string www_root;
    std::optional<int> tick_period;
    bool randomize_spawn_points = false;
};

inline std::optional<ServerArgs> ParseCommandLine(int argc, char* argv[]) {
    po::options_description desc("Allowed options");
    
    ServerArgs args;
    
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<int>(), "set tick period")
        ("config-file,c", po::value<std::string>(&args.config_file)->required(), "set config file path")
        ("www-root,w", po::value<std::string>(&args.www_root)->required(), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions");
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return std::nullopt;
    }
    
    po::notify(vm);
    
    if (vm.count("tick-period")) {
        args.tick_period = vm["tick-period"].as<int>();
        if (args.tick_period.value() < 0) {
            throw std::runtime_error("Tick period must be non-negative");
        }
    }
    
    return args;
}
