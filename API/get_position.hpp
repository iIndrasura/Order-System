#include "utils/performRequest.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <string>
#include <vector>
#include <iostream>
#include "InstrumentListFetcher.hpp"
#include "../Overlay/Renderer.hpp"
#include <thread>

class GetPosition {

private:
    using json = nlohmann::json;

    std::string accessToken;
    std::vector<std::string> instrumentNames; // Store instrument names here
    std::string selectedInstrument;           // Currently selected instrument
    json positionResponse;                    // Store the API response here
    bool showResponsePopup = false;           // Flag to show/hide response popup
    std::atomic<bool> isLoading = false;      // Flag to indicate if the API call is in progress


public:
    GetPosition(const std::string& token)
        : accessToken(token) {
        loadInstruments();
    }

    void Render() 
    {
        ImGui::Begin("Get Position", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginCombo("Instrument", selectedInstrument.empty() ? "Select an instrument" : selectedInstrument.c_str())) {
            for (const auto& instrument : instrumentNames) {
                bool isSelected = (selectedInstrument == instrument);
                if (ImGui::Selectable(instrument.c_str(), isSelected)) {
                    selectedInstrument = instrument;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Fetch position button
        if (ImGui::Button("Get Position") && !selectedInstrument.empty() && !isLoading) {
            std::thread(&GetPosition::fetchPosition, this).detach();
        }

        if (isLoading) {
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }

        // Response in a popup window
        if (showResponsePopup) {
            ImGui::OpenPopup("Position Response");
            if (ImGui::BeginPopupModal("Position Response", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Position Response:\n%s", positionResponse.dump(4).c_str());
                if (ImGui::Button("Close")) {
                    showResponsePopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }

private:
    void loadInstruments() 
    {
        isLoading = true;
        InstrumentListFetcher instrumentFetcher;
        instrumentNames = instrumentFetcher.fetchInstrumentNames();
        isLoading = false;
    }

    void fetchPosition() 
    {
        isLoading = true;
        std::string url = "https://test.deribit.com/api/v2/private/get_position";

        json payload = {
            {"jsonrpc", "2.0"},
            {"id", 8},
            {"method", "private/get_position"},
            {"params", {{"instrument_name", selectedInstrument}}}
        };
        std::string payloadStr = payload.dump();

        Request request;
        std::string responseStr = request.performRequest(url, payloadStr, accessToken);

        if (!responseStr.empty()) {
            positionResponse = json::parse(responseStr);
            showResponsePopup = true;
        }
        else {
            std::cerr << "Error fetching position: Empty response from API" << std::endl;
        }

        isLoading = false;
    }
};
