#include "network.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../vendor/httplib.h"
#include <iostream>

Network::Network(std::string url) : base_url(url) {}

void Network::setToken(std::string token) {
    auth_token = token;
}

std::string Network::post(std::string path, std::string json_body) {
    // Requirement:
    // 1. Send HTTP POST request to 'path' with 'json_body'.
    // 2. Include Authorization header if token is set.
    // 3. Return response body if status is 200.
    // 4. Handle errors and return empty string on failure.
    return "";
}

std::string Network::get(std::string path) {
    // Requirement:
    // 1. Send HTTP GET request to 'path'.
    // 2. Include Authorization header if token is set.
    // 3. Return response body if status is 200.
    // 4. Handle errors and return empty string on failure.
    return "";
}