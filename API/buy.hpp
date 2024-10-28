#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <imgui.h>
#include <thread>
#include "utils/performRequest.hpp"
#include "InstrumentListFetcher.hpp"
#include "../Overlay/Renderer.hpp"

class PlaceOrder {
private:
    using json = nlohmann::json;
    Request request;

    json responseBuffer;
    std::string accessToken;
    float amount = 0;
    int contracts = 0;
    std::string type = "all";
    std::string label;
    float price = 0;
    bool show_price = false;

    std::vector<std::string> instrumentNames;
    std::string selectedInstrument;
    std::atomic<bool> isLoading = false;

    // Variables for displaying messages
    std::string errorMessage;
    std::string successMessage;

public:
    PlaceOrder(const std::string& token) : accessToken(token) {
        loadInstruments();
    }

    // Rendering function for the order placement UI
    void Render() {
        ImGui::Begin("Place Order", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // Dropdown for instruments
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

        // Input for Amount
        ImGui::InputFloat("Amount", &amount);

        // Input for Contracts
        ImGui::InputInt("Contracts", &contracts);

        // Dropdown for Type
        const char* typeItems[10] = { "all", "limit", "stop_all", "stop_limit", "stop_market",
                                      "take_all", "take_limit", "take_market", "trailing_all", "trailing_stop" };
        if (ImGui::BeginCombo("Type", type.c_str())) {
            for (int i = 0; i < IM_ARRAYSIZE(typeItems); i++) {
                bool is_selected = (type == typeItems[i]);
                if (ImGui::Selectable(typeItems[i], is_selected)) {
                    type = typeItems[i];
                    show_price = (type == "limit" || type == "stop_limit");
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Input for Label
        char labelBuffer[128];
        strncpy_s(labelBuffer, sizeof(labelBuffer), label.c_str(), _TRUNCATE);

        if (ImGui::InputText("Label", labelBuffer, sizeof(labelBuffer))) {
            label = labelBuffer;
        }

        // Show Price input if type requires it
        if (show_price) {
            ImGui::InputFloat("Price", &price);
        }

        // Button to submit the order
        if (ImGui::Button("Place Order")) {
            errorMessage.clear();
            successMessage.clear();
            if (validateInputs()) {
                std::thread(&PlaceOrder::sendOrderRequest, this).detach();
            }
            else {
                errorMessage = "Error: Please fill in all required fields.";
            }
        }

        // Display success or error messages
        if (!errorMessage.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color for error
            ImGui::Text("%s", errorMessage.c_str());
            ImGui::PopStyleColor();
        }

        if (!successMessage.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green color for success
            ImGui::Text("%s", successMessage.c_str());
            ImGui::PopStyleColor();
        }

        // Show loading animation
        if (isLoading) {
            ImGui::Text("Loading...");
            ImGui::SameLine();
            Renderer::Spinner("##spinner", 15.0f, 6.0f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
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

    // Validate inputs based on business rules
    bool validateInputs() {
        if (selectedInstrument.empty() || type.empty() || label.empty()) return false;
        if (amount <= 0 && contracts <= 0) return false;
        if (show_price && price <= 0) return false;
        return true;
    }

    // Function to create the payload and send the order request
    void sendOrderRequest() {
        isLoading = true;
        // Create JSON payload
        nlohmann::json payload = {
            {"method", "private/buy"},
            {"jsonrpc", "2.0"},
            {"id", 57},
            {"params", {
                {"instrument_name", selectedInstrument},
                {"type", type},
                {"label", label}
            }}
        };

        // Optional fields based on conditions
        if (amount > 0) payload["params"]["amount"] = amount;
        if (contracts > 0) payload["params"]["contracts"] = contracts;
        if (show_price) payload["params"]["price"] = price;

        // Make the API request
        std::string url = "https://test.deribit.com/api/v2/private/buy";
        auto response = request.performRequest(url, payload.dump(), accessToken);

        try {
            responseBuffer = json::parse(response);
        }
        catch (const json::parse_error& e) {
            errorMessage = "Error parsing JSON: " + std::string(e.what());
            isLoading = false;
            return;
        }

        // Handle the response
        if (responseBuffer.contains("error")) {
            errorMessage = "Error: " + responseBuffer["error"]["message"].get<std::string>() +
                " (" + responseBuffer["error"]["data"]["param"].get<std::string>() + ": " +
                responseBuffer["error"]["data"]["reason"].get<std::string>() + ")";
        }
        else if (responseBuffer.contains("result") && responseBuffer["result"].contains("order")) {
            const auto& order = responseBuffer["result"]["order"];
            successMessage = "Order placed successfully!\nOrder ID: " + order["order_id"].get<std::string>() +
                "\nStatus: " + order["order_state"].get<std::string>();
        }
        else {
            errorMessage = "Unexpected response format.";
        }

        isLoading = false;
    }
};
