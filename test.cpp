#include "include/gemini_engine.h"
#include <iostream>
#include <fstream>
#include <string>

std::string get_api_key_from_env() {
    std::ifstream file(".env");
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.substr(0, 14) == "GEMINI_API_KEY") {
                return line.substr(15);
            }
        }
    }
    return "";
}

int main() {
    std::string api_key = get_api_key_from_env();

    if (api_key.empty()) {
        std::cerr << "Error: API Key not found in .env file." << std::endl;
        return 1;
    }
    
    try {
        GeminiEngine engine(api_key);
        
        PromptConfig config;
        config.model_name = "gemini-2.5-flash";
        config.system_instruction = "Bạn là một trợ lý AI nói tiếng Việt ngắn gọn. Mọi câu trả lời của bạn BẮT ĐẦU bằng từ 'HELLOOOO'.";
        
        std::vector<ContentPart> parts;
        std::string image_path = "temp_screenshot.png";
        if (std::filesystem::exists(image_path)) {
            std::string base64_image = encode_file_to_base64(image_path);
            parts.push_back({ContentPart::Type::IMAGE, base64_image, "image/png"});
            parts.push_back({ContentPart::Type::TEXT, "Hãy mô tả bức ảnh này.", ""});
        } else {
            parts.push_back({ContentPart::Type::TEXT, "Chào bạn, hãy giới thiệu bản thân.", ""});
        }
        
        std::cout << "Sending request to Gemini with System Prompt: \"" << config.system_instruction << "\"" << std::endl;
        std::string response = engine.generate(parts, config);
        
        std::cout << "\nResponse:\n" << response << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

