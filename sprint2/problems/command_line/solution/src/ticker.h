#pragma once

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <functional>
#include <memory>
#include <chrono>

namespace net = boost::asio;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_(strand)
        , period_(period)
        , handler_(std::move(handler))
        , timer_(strand) {
    }

    void Start() {
        last_tick_ = Clock::now();
        net::dispatch(strand_, [self = shared_from_this()] {
            self->ScheduleTick();
        });
    }

    void Stop() {
        net::dispatch(strand_, [self = shared_from_this()] {
            self->timer_.cancel();
        });
    }

private:
    void ScheduleTick() {
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](boost::system::error_code ec) {
            self->OnTick(ec);
        });
    }

    void OnTick(boost::system::error_code ec) {
        using namespace std::chrono;
        
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        
        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            try {
                handler_(delta);
            } catch (...) {
            }
            ScheduleTick();
        }
    }

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_;
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};
