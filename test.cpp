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
        config.model_name = "gemini-3.1-flash-lite";
        
        // Path supports Unicode directly
        std::string base64_image = encode_file_to_base64("C:\\Users\\ADMIN\\OneDrive\\Ảnh\\Screenshots\\Screenshot_20260411_104849_Facebook.jpg");

        std::vector<ContentPart> parts = {
            {ContentPart::Type::IMAGE, base64_image, "image/jpeg"},
            {ContentPart::Type::TEXT, "Hãy bình phẩm về tác phẩm (ảnh) này.", ""}
        };
        
        std::cout << "Sending request to Gemini..." << std::endl;
        std::string response = engine.generate(parts, config);
        
        std::cout << "\nResponse:\n" << response << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

