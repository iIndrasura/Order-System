#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <imgui.h>
#include <thread>
#include <future>
#include "../Overlay/Renderer.hpp"
#include "utils/performRequest.hpp"

class GetOpenOrder
{
private:
    using json = nlohmann::json;

    json apiResponse;                        // Stores the API response
    std::atomic<bool> isLoading = false;     // Flag to indicate if the API call is in progress
    bool showResponseWindow = false;         // Controls showing the response window
    bool closeWindow = true;                 // Controls closing the response window
    std::string selectedOrderId;             // Selected order ID for editing or cancelling
    int selectedIndex = -1;                  // Index of the selected checkbox

    Request request;                         // Instance of Request class

    // Dropdown values
    const char* kindItems[5] = { "future", "future_combo", "option", "option_combo", "spot" };
    const char* typeItems[10] = { "all", "limit", "stop_all", "stop_limit", "stop_market",
                                  "take_all", "take_limit", "take_market", "trailing_all", "trailing_stop" };
    int selectedKind = 0;
    int selectedType = 0;

    std::string accessToken;

public:

    GetOpenOrder(const std::string& token)
        : accessToken(token) {
    }

    void Render()
    {
        ImGui::Begin("Open Orders", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Combo("Kind", &selectedKind, kindItems, IM_ARRAYSIZE(kindItems));
        ImGui::Combo("Type", &selectedType, typeItems, IM_ARRAYSIZE(typeItems));

        if (ImGui::Button("Get Open Orders")) {
            showResponseWindow = false;
            closeWindow = true;
            selectedIndex = -1;

            std::thread(&GetOpenOrder::fetchOpenOrders, this).detach();
        }

        if (isLoading) {
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }

        // Display the open orders in a table with checkboxes
        if (showResponseWindow && closeWindow) {
            ImGui::Begin("Open Orders Table", &closeWindow);
            displayOrderTableWithCheckbox();
            ImGui::End();
        }

        ImGui::End();
    }

    // Retrieve the selected order ID
    std::string getSelectedOrderId() const {
        return selectedOrderId;
    }

    // Separate render function to display open orders in a simple table without checkboxes
    void RenderOrderTableDisplay()
    {
        if (showResponseWindow) {
            ImGui::Begin("Open Orders Display", &closeWindow);
            displayOrderTableSimple();
            ImGui::End();
        }
    }

private:
    void fetchOpenOrders() 
    {
        isLoading = true;

        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/get_open_orders"},
            {"params", {
                {"kind", kindItems[selectedKind]},
                {"type", typeItems[selectedType]}
            }},
            {"id", 33}
        };

        std::string url = "https://test.deribit.com/api/v2/private/get_open_orders";
        std::string responseBuffer = request.performRequest(url, payload.dump(), accessToken);

        try {
            apiResponse = json::parse(responseBuffer);
            showResponseWindow = true;
            isLoading = false;
        }
        catch (const json::parse_error& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }
    }

    void displayOrderTableWithCheckbox()
    {
        ImGui::Text("Select an order to get the Order ID");

        if (apiResponse.contains("result") && apiResponse["result"].is_array()) {
            ImGui::Columns(6, "orderTable"); // 6 columns

            ImGui::Text("Select"); ImGui::NextColumn();
            ImGui::Text("Order ID"); ImGui::NextColumn();
            ImGui::Text("Instrument"); ImGui::NextColumn();
            ImGui::Text("Direction"); ImGui::NextColumn();
            ImGui::Text("Price"); ImGui::NextColumn();
            ImGui::Text("Contracts"); ImGui::NextColumn();
            ImGui::Separator();

            int index = 0;
            for (const auto& order : apiResponse["result"]) {
                bool selected = (index == selectedIndex);
                if (ImGui::Checkbox(("##checkbox" + std::to_string(index)).c_str(), &selected)) {
                    if (selected) {
                        selectedIndex = index;
                        selectedOrderId = order["order_id"].get<std::string>();
                    }
                    else {
                        selectedIndex = -1;
                        selectedOrderId = "";
                    }
                }
                ImGui::NextColumn();
                ImGui::Text("%s", order["order_id"].get<std::string>().c_str()); ImGui::NextColumn();
                ImGui::Text("%s", order["instrument_name"].get<std::string>().c_str()); ImGui::NextColumn();
                ImGui::Text("%s", order["direction"].get<std::string>().c_str()); ImGui::NextColumn();
                ImGui::Text("%.2f", order["price"].get<double>()); ImGui::NextColumn();
                ImGui::Text("%d", order["contracts"].get<int>()); ImGui::NextColumn();
                index++;
            }
            ImGui::Columns(1);
        }
    }

    // Displays open orders in a simple table format
    void displayOrderTableSimple()
    {
        if (apiResponse.contains("result") && apiResponse["result"].is_array()) {
            ImGui::Columns(5, "orderTableDisplay");

            ImGui::Text("Order ID"); ImGui::NextColumn();
            ImGui::Text("Instrument"); ImGui::NextColumn();
            ImGui::Text("Direction"); ImGui::NextColumn();
            ImGui::Text("Price"); ImGui::NextColumn();
            ImGui::Text("Contracts"); ImGui::NextColumn();
            ImGui::Separator();

            for (const auto& order : apiResponse["result"]) {
                ImGui::Text("%s", order["order_id"].get<std::string>().c_str()); ImGui::NextColumn();
                ImGui::Text("%s", order["instrument_name"].get<std::string>().c_str()); ImGui::NextColumn();
                ImGui::Text("%s", order["direction"].get<std::string>().c_str()); ImGui::NextColumn();
                ImGui::Text("%.2f", order["price"].get<double>()); ImGui::NextColumn();
                ImGui::Text("%d", order["contracts"].get<int>()); ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
    }
};
