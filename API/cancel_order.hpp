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


class CancelOrder {
private:

    using json = nlohmann::json;

    Request request;
    std::string accessToken;
    int selectedIndex = -1;           // Index for selected order
    std::string selectedOrderId = "";  // Selected order ID
    json openOrderResponse;            // API response for fetching open orders
    json cancelResponse;                // API response for fetching cancel orders
    bool showCancellationMessage = false; // Flag to show cancellation message
    bool cancellationError = false;      // Flag to show cancellation error
    bool noOpenOrders = false;           // Flag to indicate no open orders
    std::atomic<bool> isLoading = false;

public:
    CancelOrder(const std::string& token)
        : accessToken(token), noOpenOrders(false) {}

    void Render() 
    {
        ImGui::Begin("Cancel Order", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        const char* kindItems[5] = { "future", "future_combo", "option", "option_combo", "spot" };
        static int selectedKind = 0;
        ImGui::Combo("Kind", &selectedKind, kindItems, IM_ARRAYSIZE(kindItems));

        const char* typeItems[10] = { "all", "limit", "stop_all", "stop_limit", "stop_market",
                                       "take_all", "take_limit", "take_market", "trailing_all", "trailing_stop" };
        static int selectedType = 0;
        ImGui::Combo("Type", &selectedType, typeItems, IM_ARRAYSIZE(typeItems));

        // Button to fetch open orders
        if (ImGui::Button("Get Open Orders")) {
            fetchOpenOrders(kindItems[selectedKind], typeItems[selectedType]);
        }

        // Display order table
        displayOrderTableWithCheckbox();

        // Show message if there are no open orders
        if (noOpenOrders) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color
            ImGui::Text("No open orders found.");
            ImGui::PopStyleColor();
        }

        // Cancel button
        if (ImGui::Button("Cancel Selected Order") && !selectedOrderId.empty()) {
            std::thread(&CancelOrder::cancelSelectedOrder, this).detach();
        }

        if (isLoading) {
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }

        // cancellation message
        if (showCancellationMessage) {
            ImGui::Text("Order %s has been cancelled successfully.", selectedOrderId.c_str());
        }
        else if (cancellationError) {
            ImGui::Text("Error cancelling order: %s", cancelResponse["error"].dump().c_str());
        }

        ImGui::End();
    }

private:
    void fetchOpenOrders(const char* kind, const char* type) 
    {
        isLoading = true;

        json payload = {
            {"method", "private/get_open_orders"},
            {"params", {
                {"kind", kind},
                {"type", type}
            }},
            {"jsonrpc", "2.0"},
            {"id", 33}
        };

        std::string url = "https://test.deribit.com/api/v2/private/get_open_orders";
        std::string responseBuffer = request.performRequest(url, payload.dump(), accessToken);

        try {
            openOrderResponse = json::parse(responseBuffer);
        }
        catch (const json::parse_error& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }

        // Response contains errors
        if (openOrderResponse.contains("error")) {
            std::cerr << "Error fetching open orders: " << openOrderResponse["error"].dump() << std::endl;
            noOpenOrders = true; // Set to true if there’s an error
            isLoading = false;
            return; // Return early if there's an error
        }

        if (openOrderResponse.contains("result") && openOrderResponse["result"].is_array()) {
            if (openOrderResponse["result"].empty()) {
                std::cout << "No open orders found." << std::endl;
                noOpenOrders = true;
                isLoading = false;
            }
            else {
                noOpenOrders = false;
                isLoading = false;
            }
        }
        else {
            std::cerr << "Invalid response format: Missing or invalid 'result' field." << std::endl;
            noOpenOrders = true;
            isLoading = false;
        }
    }

    // Function to display orders with checkboxes
    void displayOrderTableWithCheckbox() 
    {
        ImGui::Text("Select an order to cancel:");

        // Check if open orders exist before displaying
        if (openOrderResponse.contains("result") && openOrderResponse["result"].is_array() && !noOpenOrders) {
            ImGui::Columns(6, "orderTable"); // 6 columns

            ImGui::Text("Select"); ImGui::NextColumn();
            ImGui::Text("Order ID"); ImGui::NextColumn();
            ImGui::Text("Instrument"); ImGui::NextColumn();
            ImGui::Text("Direction"); ImGui::NextColumn();
            ImGui::Text("Price"); ImGui::NextColumn();
            ImGui::Text("Contracts"); ImGui::NextColumn();
            ImGui::Separator();

            int index = 0;
            for (const auto& order : openOrderResponse["result"]) {
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

    // Function to cancel the selected order
    void cancelSelectedOrder() 
    {
        isLoading = true;

        json payload = {
            {"method", "private/cancel"},
            {"params", {
                {"order_id", selectedOrderId}
            }},
            {"jsonrpc", "2.0"},
            {"id", 36}
        };

        std::string url = "https://test.deribit.com/api/v2/private/cancel";
        std::string responseBuffer = request.performRequest(url, payload.dump(), accessToken);

        try {
            cancelResponse = json::parse(responseBuffer);
        }
        catch (const json::parse_error& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }

        // Response contains errors
        if (cancelResponse.contains("error")) {
            std::cerr << "Error cancelling order: " << cancelResponse["error"].dump() << std::endl;
            showCancellationMessage = false;
            cancellationError = true;
            return;
        }

        // cancellation was successful
        if (cancelResponse.contains("result") && cancelResponse["result"]["order_state"] == "cancelled") {
            showCancellationMessage = true;
            cancellationError = false;
        }
        else {
            std::cerr << "Failed to cancel order: " << cancelResponse.dump() << std::endl;
            showCancellationMessage = false;
            cancellationError = true;
        }

        isLoading = false;
    }
};
