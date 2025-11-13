#pragma once
#include "http_server.h"
#include "application.h"
#include "join_handler.h"
#include "players_handler.h"
#include "game_state_handler.h"
#include "player_action_handler.h"
#include "tick_handler.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    RequestHandler(app::Application& application, 
                   net::strand<net::io_context::executor_type> strand,
                   bool is_auto_tick_mode = false)
        : application_(application)
        , strand_(strand)
        , is_auto_tick_mode_(is_auto_tick_mode)
        , join_handler_(application)
        , players_handler_(application)
        , game_state_handler_(application)
        , player_action_handler_(application)
        , tick_handler_(application) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

private:
    app::Application& application_;
    net::strand<net::io_context::executor_type> strand_;
    bool is_auto_tick_mode_;
    JoinHandler join_handler_;
    PlayersHandler players_handler_;
    GameStateHandler game_state_handler_;
    PlayerActionHandler player_action_handler_;
    TickHandler tick_handler_;
    
    template <typename Body, typename Allocator, typename Send>
    void HandleFileRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);
    
    template <typename Body, typename Allocator, typename Send>
    void HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);
    
    template <typename Body, typename Allocator, typename Send>
    void HandleGetMapsList(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);
    
    template <typename Body, typename Allocator, typename Send>
    void HandleGetMap(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

    void SendBadRequest(Send&& send, std::string message);
    
    template <typename Request>
    void SendErrorResponse(Request&& req, Send&& send, 
                          http::status status, 
                          std::string code, std::string message);

    std::string ExtractMapId(std::string_view path);
};

} // namespace http_handler