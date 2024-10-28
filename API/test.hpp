#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <imgui.h>
#include <thread>
#include <future>
#include "../Overlay/Renderer.hpp"
#include "utils/performRequest.hpp"

class TestAPI
{
private:
    using json = nlohmann::json;

    json apiResponse;                    // Variable to store the API response
    std::atomic<bool> isLoading = false; // Flag to indicate if the API call is in progress
    bool showResponseWindow = false;     // Flag to control when to show the response window
    bool closeWindow = true;             // Control when to close the response window

    Request request;                     // Create an instance of the Request class

public:
    void Render()
    {
        ImGui::Begin("Deribit API Test", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button("Test API Connection")) {
            // Reset flags to ensure window reappears after closing
            showResponseWindow = false;
            closeWindow = true;

            // Trigger the API call in a separate thread when button is clicked
            std::thread(&TestAPI::fetchApiResponse, this).detach();
        }

        if (isLoading) {
            // Show loading animation (simple rotating lines)
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }

        // If the API call is done and we have a response, show a new window with the result
        if (showResponseWindow && closeWindow) {
            ImGui::Begin("API Response", &closeWindow);
            ImGui::Text("Response from Deribit API:");
            ImGui::Separator();

            // Display the formatted JSON response
            std::string responseStr = apiResponse.dump(4);  // Format the response nicely
            ImGui::TextWrapped("%s", responseStr.c_str());

            ImGui::End();
        }

        ImGui::End();
    }

private:
    // Function to handle the API call in a separate thread
    void fetchApiResponse() 
    {
        isLoading = true;

        // Create JSON payload
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "public/test"},
            {"params", json::object()},
            {"id", 3}
        };

        // Perform the request
        std::string url = "https://test.deribit.com/api/v2/public/test";
        std::string responseBuffer = request.performRequest(url, payload.dump());

        // Parse response
        try {
            apiResponse = json::parse(responseBuffer);
        }
        catch (const json::parse_error& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }

        isLoading = false;
        showResponseWindow = true;
    }
};