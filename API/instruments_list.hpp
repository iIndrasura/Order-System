#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <imgui.h>
#include <thread>
#include <future>
#include <atomic>
#include "../Overlay/Renderer.hpp"
#include "utils/performRequest.hpp"

class InstrumentAPI
{
private:
    using json = nlohmann::json;

    json apiResponse;                    // Variable to store the API response
    std::atomic<bool> isLoading = false; // Flag to indicate if the API call is in progress
    bool showResponseWindow = false;     // Flag to control when to show the response window
    bool closeWindow = false;            // Tracks window close status

public:
    void Render()
    {
        ImGui::Begin("Deribit Instrument List", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button("Fetch Instrument List")) {
            closeWindow = true;
            showResponseWindow = false;
            std::thread(&InstrumentAPI::fetchInstrumentList, this).detach();
        }

        if (isLoading) {
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }

        if (showResponseWindow && closeWindow) {
            ImGui::Begin("Instrument List Response", &closeWindow);
            ImGui::Text("Response from Deribit API:");
            ImGui::Separator();

            std::string responseStr = apiResponse.dump(4);
            ImGui::TextWrapped("%s", responseStr.c_str());

            ImGui::End();
        }

        ImGui::End();
    }

private:
    void fetchInstrumentList() 
    {
        isLoading = true;                // Set loading flag to true
        apiResponse = getInstrumentList(); // Make the API call
        isLoading = false;               // Reset loading flag once done
        showResponseWindow = true;       // Trigger the response window to show
        closeWindow = true;              // Allow reopening if the button is clicked again
    }

    // Function to get the instrument list
    json getInstrumentList() {
        Request request;
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "public/get_instruments"},
            {"params", {{"currency", "any"}}},
            {"id", 10}
        };

        std::string url = "https://test.deribit.com/api/v2/public/get_instruments";
        std::string response = request.performRequest(url, payload.dump());

        if (!response.empty()) {
            try {
                return json::parse(response);
            }
            catch (const std::exception& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
            }
        }
        else {
            std::cerr << "performRequest failed or returned an empty response" << std::endl;
        }

        return {};
    }
};
