#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "imgui.h"
#include "utils/performRequest.hpp"
#include "../Overlay/Renderer.hpp"

class OrderBook {
private:
    Request request;
    std::string responseBuffer;
    nlohmann::json jsonResponse;

    std::vector<std::string> instrumentNames;
    std::string selectedInstrument;

    int selectedDepth = 0;
    std::vector<int> depths = { 1, 10, 100, 1000, 10000, 20, 5, 50 };

    std::atomic<bool> isLoading = false;
    bool showResponseWindow = false;
    bool showDetailsWindowFlag = false;

public:
    OrderBook() {
        loadInstruments();
    }

    void Render() 
    {
        ImGui::Begin("Order Book", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // Instrument dropdown
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

        // Depth dropdown
        if (ImGui::Combo("Select Depth", &selectedDepth,
            [](void* data, int idx) {
                static std::string depthStr;
                depthStr = std::to_string(static_cast<std::vector<int>*>(data)->at(idx));
                return depthStr.c_str();
            },
            &depths, depths.size())) {
        }

        // Button to make API call
        if (ImGui::Button("Get Order Book")) {
            std::thread(&OrderBook::fetchOrderBook, this).detach();
        }

        if (isLoading) {
            // Show loading animation
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }

        // Response window
        if (showResponseWindow) {
            ImGui::Begin("API Response", &showResponseWindow);
            ImGui::Text("Response from Deribit API:");
            ImGui::Separator();
            std::string responseStr = jsonResponse.dump(4);
            ImGui::TextWrapped("%s", responseStr.c_str());
            ImGui::End();
        }

        // Button to show detailed view
        if (!responseBuffer.empty()) {
            if (ImGui::Button("Show Details")) {
                showDetailsWindowFlag = true;
            }
        }

        // Details window
        if (showDetailsWindowFlag) {
            showDetailsWindow();
        }

        ImGui::End();
    }

private:
    void loadInstruments() {
        isLoading = true;
        InstrumentListFetcher instrumentFetcher;
        instrumentNames = instrumentFetcher.fetchInstrumentNames();
        isLoading = false;
    }

    void fetchOrderBook() {
        isLoading = true;
        nlohmann::json payload = {
            {"method", "public/get_order_book"},
            {"params", {
                {"instrument_name", selectedInstrument},
                {"depth", selectedDepth}
            }},
            {"jsonrpc", "2.0"},
            {"id", 27}
        };

        std::string url = "https://test.deribit.com/api/v2/public/get_order_book";
        responseBuffer = request.performRequest(url, payload.dump());

        try {
            jsonResponse = nlohmann::json::parse(responseBuffer);
        }
        catch (nlohmann::json::parse_error& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }

        isLoading = false;
        showResponseWindow = true;
    }

    void showDetailsWindow() {
        if (jsonResponse.contains("result")) {
            const auto& result = jsonResponse["result"];

            ImGui::Begin("Order Book Details", &showDetailsWindowFlag, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Order Book Details:");

            // Display key information with checks
            ImGui::Text("Instrument Name: %s", result.value("instrument_name", "N/A").c_str());
            ImGui::Text("Index Price: %.2f", result.value("index_price", 0.0f));
            ImGui::Text("Underlying Price: %.2f", result.value("underlying_price", 0.0f));
            ImGui::Text("Mark Price: %.4f", result.value("mark_price", 0.0f));
            ImGui::Text("Settlement Price: %.4f", result.value("settlement_price", 0.0f));

            // Displaying Greeks with error checks
            if (result.contains("greeks")) {
                const auto& greeks = result["greeks"];
                ImGui::Text("Greeks:");
                ImGui::Text("  Delta: %.4f", greeks.value("delta", 0.0f));
                ImGui::Text("  Gamma: %.4f", greeks.value("gamma", 0.0f));
                ImGui::Text("  Theta: %.4f", greeks.value("theta", 0.0f));
                ImGui::Text("  Vega: %.4f", greeks.value("vega", 0.0f));
                ImGui::Text("  Rho: %.4f", greeks.value("rho", 0.0f));
            }
            else {
                ImGui::Text("Greeks: N/A");
            }

            // Displaying Bids Table
            ImGui::Text("Bids:");
            if (result.contains("bids") && !result["bids"].empty()) {
                ImGui::BeginTable("Bids", 2, ImGuiTableFlags_Borders);
                ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                for (const auto& bid : result["bids"]) {
                    if (bid.is_array() && bid.size() == 2) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%.6f", bid[0].get<float>());
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", bid[1].get<int>());
                    }
                }
                ImGui::EndTable();
            }
            else {
                ImGui::Text("No Bids Available");
            }

            ImGui::End();
        }
    }
};