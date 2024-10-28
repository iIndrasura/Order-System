#include "utils/performRequest.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <chrono>

class Authenticator {
    using json = nlohmann::json;

public:
    std::string clientID;
    std::string clientSecret;
    std::string accessToken;
    std::string refreshToken; // Store refresh token
    int expiresIn = 0;        // Store expiration time
    std::string scope;        // Store the scope
    long long timestamp;      // Store the timestamp

    Authenticator(const std::string& id, const std::string& secret)
        : clientID(id), clientSecret(secret), accessToken(""), refreshToken(""), scope(""), expiresIn(0) {
        // Generate timestamp (current time in milliseconds)
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    bool authenticate() {
        // Deribit public/auth URL
        std::string url = "https://test.deribit.com/api/v2/public/auth";

        // JSON payload for client_credentials authentication
        json payload = {
            {"jsonrpc", "2.0"},
            {"id", 20},
            {"method", "public/auth"},
            {"params", {
                {"grant_type", "client_credentials"},
                {"client_id", clientID},
                {"client_secret", clientSecret},
                {"timestamp", timestamp}
            }}
        };
        std::string payloadStr = payload.dump();

        Request request;
        std::string responseStr = request.performRequest(url, payloadStr);

        if (!responseStr.empty()) {
            json response = json::parse(responseStr);

            // Check if access token is received in response
            if (response.contains("result")) {
                if (response["result"].contains("access_token")) {
                    accessToken = response["result"]["access_token"];

                    // Explicitly check if the refresh_token exists and is a string
                    if (response["result"].contains("refresh_token") &&
                        response["result"]["refresh_token"].is_string()) {
                        refreshToken = response["result"]["refresh_token"].get<std::string>();
                    }
                    else {
                        refreshToken = ""; // Assign an empty string if not found
                    }

                    // Similarly, check for expires_in
                    if (response["result"].contains("expires_in") &&
                        response["result"]["expires_in"].is_number_integer()) {
                        expiresIn = response["result"]["expires_in"].get<int>();
                    }
                    else {
                        expiresIn = 0; // Assign a default value if not found
                    }

                    // Check for scope
                    if (response["result"].contains("scope") &&
                        response["result"]["scope"].is_string()) {
                        scope = response["result"]["scope"].get<std::string>();
                    }
                    else {
                        scope = ""; // Assign an empty string if not found
                    }

                    std::cout << "Access token received: " << accessToken << std::endl;
                    std::cout << "Refresh token: " << refreshToken << std::endl;
                    std::cout << "Expires in (seconds): " << expiresIn << std::endl;
                    std::cout << "Scope: " << scope << std::endl;

                    return true;
                }
            }
        }
        else {
            std::cerr << "Failed to receive response from authentication request." << std::endl;
        }
        return false;
    }
};