#include "../include/gemini_engine.h"
#include "../include/base64.h"
#include <iostream>
#include <stdexcept>
#include <curl/curl.h>
#include <json/json.h> // Using jsoncpp
#include <fstream>
#include <sstream>
#include <filesystem>

std::string encode_file_to_base64(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Could not open file: " + path.string());
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), {});
    return base64_encode(buffer);
}

// Helper to handle CURL callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

GeminiEngine::GeminiEngine(std::string api_key) : api_key_(std::move(api_key)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

GeminiEngine::~GeminiEngine() {
    curl_global_cleanup();
}

std::string GeminiEngine::generate(const std::vector<ContentPart>& parts, const PromptConfig& config) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");

    Json::Value root;
    // Add configuration
    root["generationConfig"]["temperature"] = config.temperature;

    if (!config.system_instruction.empty()) {
        Json::Value sys_inst;
        Json::Value sys_parts(Json::arrayValue);
        Json::Value sys_part;
        sys_part["text"] = config.system_instruction;
        sys_parts.append(sys_part);
        sys_inst["parts"] = sys_parts;
        root["system_instruction"] = sys_inst;
    }

    Json::Value contents_array(Json::arrayValue);
    Json::Value content_obj;
    Json::Value parts_array(Json::arrayValue);

    for (const auto& part : parts) {
        Json::Value part_obj;
        if (part.type == ContentPart::Type::TEXT) {
            part_obj["text"] = part.data;
        } else {
            Json::Value inline_data;
            inline_data["mimeType"] = part.mime_type;
            inline_data["data"] = part.data;
            part_obj["inlineData"] = inline_data;
        }
        parts_array.append(part_obj);
    }
    content_obj["parts"] = parts_array;
    contents_array.append(content_obj);
    root["contents"] = contents_array;

    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" + config.model_name + ":generateContent?key=" + api_key_;

    std::string response_string;
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    Json::StreamWriterBuilder writer;
    std::string json_payload = Json::writeString(writer, root);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
    }

    // Parse Response
    Json::CharReaderBuilder reader;
    Json::Value j;
    std::string errs;
    std::stringstream ss(response_string);
    if (!Json::parseFromStream(reader, ss, &j, &errs)) {
        throw std::runtime_error("Failed to parse JSON response: " + errs);
    }

    // Check for API-level errors
    if (http_code != 200) {
        if (j.isMember("error") && j["error"].isMember("message")) {
            throw std::runtime_error("Gemini API Error (" + std::to_string(http_code) + "): " + j["error"]["message"].asString());
        }
        throw std::runtime_error("Gemini API Error (" + std::to_string(http_code) + "): " + response_string);
    }

    // Check for candidates
    if (j.isMember("candidates") && j["candidates"].isArray() && !j["candidates"].empty()) {
        const Json::Value& candidate = j["candidates"][0];

        // Check for content/parts/text
        if (candidate.isMember("content") && candidate["content"].isMember("parts") && candidate["content"]["parts"].isArray() && !candidate["content"]["parts"].empty()) {
             if (candidate["content"]["parts"][0].isMember("text")) {
                return candidate["content"]["parts"][0]["text"].asString();
             }
        }

        // Check finish reason if text is missing
        if (candidate.isMember("finishReason")) {
             throw std::runtime_error("Generation finished without text. Reason: " + candidate["finishReason"].asString());
        }
    }

    throw std::runtime_error("Unexpected API response format: " + response_string);
}

