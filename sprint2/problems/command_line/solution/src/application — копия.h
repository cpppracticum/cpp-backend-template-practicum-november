#pragma once
#include "http_server.h"
#include "application.h"
#include <boost/json.hpp>

namespace http_handler {

class JoinHandler {
public:
    explicit JoinHandler(app::Application& application) : application_(application) {}
    
    template <typename Body, typename Allocator, typename Send>
    void HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

private:
    app::Application& application_;
    
    struct JoinRequest {
        std::string userName;
        std::string mapId;
    };
    
    std::optional<JoinRequest> ParseJoinRequest(const std::string& body);
    void SendErrorResponse(Send&& send, http::status status, 
                          std::string code, std::string message,
                          std::string allow_header = "");
};

}  // namespace http_handler