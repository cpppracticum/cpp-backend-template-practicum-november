#include "request_handler.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

std::string RequestHandler::UrlDecode(const std::string& encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int value;
            std::istringstream iss(encoded.substr(i + 1, 2));
            iss >> std::hex >> value;
            result += static_cast<char>(value);
            i += 2;
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }
    return result;
}

std::string RequestHandler::GetMimeType(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mime_map = {
        {".htm", "text/html"}, {".html", "text/html"},
        {".css", "text/css"}, {".txt", "text/plain"},
        {".js", "text/javascript"}, {".json", "application/json"},
        {".xml", "application/xml"}, {".png", "image/png"},
        {".jpg", "image/jpeg"}, {".jpe", "image/jpeg"}, {".jpeg", "image/jpeg"},
        {".gif", "image/gif"}, {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"}, {".tif", "image/tiff"},
        {".svg", "image/svg+xml"}, {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };
    
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = mime_map.find(ext);
    if (it != mime_map.end()) return it->second;
    
    return "application/octet-stream";
}

bool RequestHandler::IsPathWithinRoot(const fs::path& path, const fs::path& root) {
    fs::path canonical_path = fs::weakly_canonical(path);
    fs::path canonical_root = fs::weakly_canonical(root);
    
    auto path_it = canonical_path.begin();
    auto root_it = canonical_root.begin();
    
    while (root_it != canonical_root.end()) {
        if (path_it == canonical_path.end() || *path_it != *root_it) return false;
        ++path_it;
        ++root_it;
    }
    return true;
}

http::response<http::file_body> RequestHandler::ServeStaticFile(const std::string& target) {
    http::response<http::file_body> response;
    
    std::string decoded = UrlDecode(target);
    std::string file_path = decoded;
    if (file_path.empty() || file_path.back() == '/') file_path += "index.html";
    if (file_path.front() == '/') file_path = file_path.substr(1);
    
    fs::path full_path = static_root_ / file_path;
    
    if (!IsPathWithinRoot(full_path, static_root_)) {
        http::response<http::string_body> error_response;
        error_response.result(http::status::bad_request);
        error_response.set(http::field::content_type, "text/plain");
        error_response.body() = "Bad request";
        error_response.prepare_payload();
        http::response<http::file_body> file_error;
        file_error.result(http::status::bad_request);
        file_error.set(http::field::content_type, "text/plain");
        return file_error;
    }
    
    if (!fs::exists(full_path) || fs::is_directory(full_path)) {
        http::response<http::string_body> error_response;
        error_response.result(http::status::not_found);
        error_response.set(http::field::content_type, "text/plain");
        error_response.body() = "File not found";
        error_response.prepare_payload();
        http::response<http::file_body> file_error;
        file_error.result(http::status::not_found);
        file_error.set(http::field::content_type, "text/plain");
        return file_error;
    }
    
    http::file_body::value_type file;
    beast::error_code ec;
    file.open(full_path.c_str(), beast::file_mode::read, ec);
    
    if (ec) {
        http::response<http::string_body> error_response;
        error_response.result(http::status::internal_server_error);
        error_response.set(http::field::content_type, "text/plain");
        error_response.body() = "Cannot open file";
        error_response.prepare_payload();
        http::response<http::file_body> file_error;
        file_error.result(http::status::internal_server_error);
        file_error.set(http::field::content_type, "text/plain");
        return file_error;
    }
    
    response.result(http::status::ok);
    response.set(http::field::content_type, GetMimeType(full_path.string()));
    response.body() = std::move(file);
    response.prepare_payload();
    
    return response;
}

http::response<http::string_body> RequestHandler::HandleApiRequest(const http::request<http::string_body>& req) {
    std::string target(req.target().data(), req.target().size());
    
    if (target == "/api/v1/maps") return HandleMapsRequest();
    else if (target.size() > 13 && target.substr(0, 13) == "/api/v1/maps/") {
        return HandleMapRequest(target.substr(13));
    }
    else if (target.size() >= 4 && target.substr(0, 4) == "/api") {
        return MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request");
    }
    return MakeErrorResponse(http::status::not_found, "notFound", "Not found");
}

http::response<http::string_body> RequestHandler::HandleMapsRequest() {
    json::array maps_array;
    for (const auto& map : game_.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_array.push_back(std::move(map_obj));
    }
    std::string body = json::serialize(maps_array);
    
    http::response<http::string_body> response(http::status::ok, 11);
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(false);
    return response;
}

http::response<http::string_body> RequestHandler::HandleMapRequest(const std::string& map_id_str) {
    model::Map::Id map_id(map_id_str);
    const model::Map* map = game_.FindMap(map_id);
    if (!map) {
        return MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found");
    }
    
    json::object map_obj;
    map_obj["id"] = *map->GetId();
    map_obj["name"] = map->GetName();
    
    json::array roads_array;
    for (const auto& road : map->GetRoads()) {
        json::object road_obj;
        auto start = road.GetStart();
        auto end = road.GetEnd();
        road_obj["x0"] = start.x;
        road_obj["y0"] = start.y;
        if (road.IsHorizontal()) road_obj["x1"] = end.x;
        else road_obj["y1"] = end.y;
        roads_array.push_back(std::move(road_obj));
    }
    map_obj["roads"] = std::move(roads_array);
    
    json::array buildings_array;
    for (const auto& building : map->GetBuildings()) {
        auto bounds = building.GetBounds();
        json::object building_obj;
        building_obj["x"] = bounds.position.x;
        building_obj["y"] = bounds.position.y;
        building_obj["w"] = bounds.size.width;
        building_obj["h"] = bounds.size.height;
        buildings_array.push_back(std::move(building_obj));
    }
    map_obj["buildings"] = std::move(buildings_array);
    
    json::array offices_array;
    for (const auto& office : map->GetOffices()) {
        json::object office_obj;
        office_obj["id"] = *office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().dx;
        office_obj["offsetY"] = office.GetOffset().dy;
        offices_array.push_back(std::move(office_obj));
    }
    map_obj["offices"] = std::move(offices_array);
    
    std::string body = json::serialize(map_obj);
    
    http::response<http::string_body> response(http::status::ok, 11);
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(false);
    return response;
}

http::response<http::string_body> RequestHandler::MakeErrorResponse(
    http::status status, const std::string& code, const std::string& message) {
    
    json::object error_obj;
    error_obj["code"] = code;
    error_obj["message"] = message;
    std::string body = json::serialize(error_obj);
    
    http::response<http::string_body> response(status, 11);
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(false);
    return response;
}

}  // namespace http_handler
