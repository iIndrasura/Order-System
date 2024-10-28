#pragma once

#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

class Request
{
private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        size_t totalSize = size * nmemb;
        userp->append((char*)contents, totalSize);
        return totalSize;
    }

public:
    std::string performRequest(const std::string& url, const std::string& payload, const std::string& token = "") 
    {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            if (!token.empty()) {
                std::string authHeader = "Authorization: Bearer " + token;
                headers = curl_slist_append(headers, authHeader.c_str());
            }

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                std::cerr << "Error in performRequest: " << curl_easy_strerror(res) << std::endl;
            }
        }
        return readBuffer;
    }
};