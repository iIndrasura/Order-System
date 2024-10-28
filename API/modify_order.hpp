#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <imgui.h>
#include "../Overlay/Renderer.hpp"
#include "utils/performRequest.hpp"
#include "../Overlay/Renderer.hpp"

using json = nlohmann::json;

class ModifyOrder {
private:
    Request request;
    std::string accessToken;
    int selectedIndex = -1;
    std::string selectedOrderId = "";
    json openOrderResponse;
    json modifyResponse;
    bool showModificationMessage = false;
    bool modificationError = false;
    bool noOpenOrders = false;
    float amount = 0.0f;
    int contracts = 0;
    std::atomic<bool> isLoading = false;

public:
    ModifyOrder(const std::string& token)
        : accessToken(token), noOpenOrders(false) {}

    void Render() 
    {
        ImGui::Begin("Modify Order", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        const char* kindItems[5] = { "future", "future_combo", "option", "option_combo", "spot" };
        static int selectedKind = 0;
        ImGui::Combo("Kind", &selectedKind, kindItems, IM_ARRAYSIZE(kindItems));

        const char* typeItems[10] = { "all", "limit", "stop_all", "stop_limit", "stop_market",
                                       "take_all", "take_limit", "take_market", "trailing_all", "trailing_stop" };
        static int selectedType = 0;
        ImGui::Combo("Type", &selectedType, typeItems, IM_ARRAYSIZE(typeItems));

        if (ImGui::Button("Get Open Orders")) {
            fetchOpenOrders(kindItems[selectedKind], typeItems[selectedType]);
        }

        displayOrderTableWithCheckbox();

        if (noOpenOrders) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("No open orders found.");
            ImGui::PopStyleColor();
        }

        if (!selectedOrderId.empty()) {
            ImGui::InputFloat("Amount", &amount);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("It represents the requested order size. For perpetual and inverse futures, the amount is in USD units. \n"
                    "For options and linear futures, it is the underlying base currency coin. \n"
                    "The `amount` is a mandatory parameter if `contracts` parameter is missing. \n"
                    "If both `contracts` and `amount` parameters are passed, they must match each other, otherwise an error is returned.");
            }

            ImGui::InputInt("Contracts", &contracts);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("It represents the requested order size in contract units and can be passed instead of `amount`. \n"
                    "The `contracts` is a mandatory parameter if `amount` parameter is missing. \n"
                    "If both `contracts` and `amount` parameters are passed, they must match each other, otherwise an error is returned.");
            }

            if (ImGui::Button("Modify Selected Order")) {
                modifySelectedOrder();
                std::thread(&ModifyOrder::modifySelectedOrder, this).detach();
            }
        }

        if (isLoading) {
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }

        if (showModificationMessage) {
            ImGui::Text("Order %s has been modified successfully.", selectedOrderId.c_str());
        }
        else if (modificationError) {
            ImGui::Text("Error modifying order: %s", modifyResponse["error"]["message"].get<std::string>().c_str());
            ImGui::Text("Error modifying order: %s", modifyResponse["error"]["data"]["reason"].get<std::string>().c_str());
        }

        ImGui::End();
    }

private:
    void fetchOpenOrders(const char* kind, const char* type) 
    {
        json payload = {
            {"method", "private/get_open_orders"},
            {"params", {{"kind", kind}, {"type", type}}},
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

        if (openOrderResponse.contains("error")) {
            noOpenOrders = true;
            return;
        }

        noOpenOrders = openOrderResponse["result"].empty();
    }

    void displayOrderTableWithCheckbox() {
        ImGui::Text("Select an order to modify:");

        if (openOrderResponse.contains("result") && openOrderResponse["result"].is_array() && !noOpenOrders) {
            ImGui::Columns(6, "orderTable");

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

    void modifySelectedOrder() 
    {
        isLoading = true;

        json payload = {
            {"method", "private/edit"},
            {"params", {{"order_id", selectedOrderId}}},
            {"jsonrpc", "2.0"},
            {"id", 47}
        };

        if (amount > 0.0f) payload["params"]["amount"] = amount;
        if (contracts > 0) payload["params"]["contracts"] = contracts;

        std::string url = "https://test.deribit.com/api/v2/private/edit";
        std::string responseBuffer = request.performRequest(url, payload.dump(), accessToken);

        try {
            modifyResponse = json::parse(responseBuffer);
        }
        catch (const json::parse_error& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }

        if (modifyResponse.contains("error")) {
            modificationError = true;
            showModificationMessage = false;
            isLoading = false;
        }
        else {
            showModificationMessage = true;
            modificationError = false;
            isLoading = false;
        }
    }
};
