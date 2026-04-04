#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <string>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

inline void InitLogger() {
    logging::add_common_attributes();
    
    logging::add_console_log(
        std::cout,
        keywords::format = "%TimeStamp%: %Message%",
        keywords::auto_flush = true
    );
    
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
}

inline void LogStartup(const std::string& address, unsigned short port) {
    BOOST_LOG_TRIVIAL(info) << "server started on " << address << ":" << port;
}

inline void LogShutdown(int code, const std::string& exception = "") {
    if (exception.empty()) {
        BOOST_LOG_TRIVIAL(info) << "server exited with code " << code;
    } else {
        BOOST_LOG_TRIVIAL(info) << "server exited with code " << code << ": " << exception;
    }
}
