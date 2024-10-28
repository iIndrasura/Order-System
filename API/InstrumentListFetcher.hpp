#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>
#include "utils/performRequest.hpp"

class InstrumentListFetcher {
private:
    using json = nlohmann::json;

public:
    std::vector<std::string> fetchInstrumentNames() 
    {
        std::vector<std::string> instrumentNames;
        std::string url = "https://test.deribit.com/api/v2/public/get_instruments";
        std::string payload = R"({
            "jsonrpc": "2.0",
            "id": 10,
            "method": "public/get_instruments",
            "params": {
                "currency": "any"
            }
        })";

        Request request;
        std::string response = request.performRequest(url, payload);

        if (!response.empty()) {
            try {
                json jsonResponse = json::parse(response);

                // Extract instrument names from the response
                if (jsonResponse.contains("result")) {
                    for (const auto& instrument : jsonResponse["result"]) {
                        if (instrument.contains("instrument_name")) {
                            instrumentNames.push_back(instrument["instrument_name"].get<std::string>());
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to parse instrument list response: " << e.what() << std::endl;
            }
        }

        return instrumentNames;
    }
};