#include "join_handler.h"
#include <boost/json.hpp>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

template <typename Body, typename Allocator, typename Send>
void JoinHandler::HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    if (req.method() != http::verb::post) {
        SendErrorResponse(std::forward<Send>(send), 
                         http::status::method_not_allowed,
                         "invalidMethod", 
                         "Only POST method is expected",
                         "POST");
        return;
    }
    auto content_type = req.find(http::field::content_type);
    if (content_type == req.end() || content_type->value() != "application/json") {
        SendErrorResponse(std::forward<Send>(send),
                         http::status::bad_request,
                         "invalidArgument",
                         "Invalid content type");
        return;
    }
    auto join_request = ParseJoinRequest(req.body());
    if (!join_request) {
        SendErrorResponse(std::forward<Send>(send),
                         http::status::bad_request,
                         "invalidArgument", 
                         "Join game request parse error");
        return;
    }
    if (join_request->userName.empty()) {
        SendErrorResponse(std::forward<Send>(send),
                         http::status::bad_request,
                         "invalidArgument",
                         "Invalid name");
        return;
    }
    model::Map::Id map_id{join_request->mapId};
    const auto* map = application_.GetGame().FindMap(map_id);
    if (!map) {
        SendErrorResponse(std::forward<Send>(send),
                         http::status::not_found,
                         "mapNotFound",
                         "Map not found");
        return;
    }
    auto result = application_.JoinGame(join_request->userName, join_request->mapId);
    if (!result) {
        SendErrorResponse(std::forward<Send>(send),
                         http::status::internal_server_error,
                         "joinFailed",
                         "Failed to join game");
        return;
    }
    json::object response;
    response["authToken"] = result->auth_token;
    response["playerId"] = *result->player_id;
    
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    res.body() = json::serialize(response);
    res.prepare_payload();
    
    send(std::move(res));
}
std::optional<JoinHandler::JoinRequest> JoinHandler::ParseJoinRequest(const std::string& body) {
    try {
        auto json_value = json::parse(body);
        auto& obj = json_value.as_object();
        
        JoinRequest request;
        request.userName = std::string(obj.at("userName").as_string());
        request.mapId = std::string(obj.at("mapId").as_string());
        
        return request;
    } catch (...) {
        return std::nullopt;
    }
}
void JoinHandler::SendErrorResponse(Send&& send, http::status status, 
                                   std::string code, std::string message,
                                   std::string allow_header) {
    json::object error_json;
    error_json["code"] = code;
    error_json["message"] = message;
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    if (!allow_header.empty()) {
        res.set(http::field::allow, allow_header);
    }
    res.body() = json::serialize(error_json);
    res.prepare_payload();
    
    send(std::move(res));
}
template void JoinHandler::HandleRequest<http::string_body, std::allocator<char>, 
    std::function<void(http::response<http::string_body>&&)>>(
    http::request<http::string_body, http::basic_fields<std::allocator<char>>>&&, 
    std::function<void(http::response<http::string_body>&&)>&&);

}  // namespace http_handler