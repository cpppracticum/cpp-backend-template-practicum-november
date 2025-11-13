#pragma once
#include "http_server.h"
#include "application.h"  
#include "model.h"  

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;

    class JoinHandler {  
    public:
        explicit JoinHandler(app::Application& application)
            : application_(application) {
        }

        template <typename Body, typename Allocator, typename Send>
        void HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

    private:
        struct JoinRequest {
            std::string userName;
            std::string mapId;
        };

        std::optional<JoinRequest> ParseJoinRequest(const std::string& body);

        template <typename Send>
        void SendErrorResponse(Send&& send, http::status status,
            std::string code, std::string message,
            std::string allow_header = "");

        app::Application& application_;
    };

}  // namespace http_handler