/**
 * @file gemini_engine.h
 * @brief Header-only contract for Gemini API Wrapper.
 * 
 * Provides a stateless, multi-modal interface for interacting with Gemini API.
 */

#ifndef GEMINI_ENGINE_H
#define GEMINI_ENGINE_H

#include <string>
#include <vector>
#include <filesystem>

/**
 * @brief Helper to read a binary file and encode it to base64.
 * @param path The filesystem path to the file.
 * @return A base64-encoded string representation of the file content.
 */
std::string encode_file_to_base64(const std::filesystem::path& path);
/**
 * @brief Represents different types of content that can be sent to Gemini.
 */
struct ContentPart {
    enum class Type { TEXT, FILE, IMAGE };
    
    Type type;
    std::string data;      ///< Content body (text or file path)
    std::string mime_type; ///< MIME type for binary data (e.g., "image/png")
};

/**
 * @brief Configuration for the generation request.
 */
struct PromptConfig {
    std::string model_name = "gemini-1.5-flash";
    float temperature = 0.7f;
};

/**
 * @brief GeminiEngine class providing a stateless interface to Gemini API.
 */
class GeminiEngine {
public:
    /**
     * @brief Constructs the engine with an API Key.
     * @param api_key The Gemini API Key.
     */
    explicit GeminiEngine(std::string api_key);

    /**
     * @brief Destructor to cleanup global CURL state.
     */
    ~GeminiEngine();
    /**
     * @brief Generates content based on a list of multi-modal parts.
     * 
     * This method is stateless; it performs the request and returns the result.
     * 
     * @param parts A vector of content parts (text, files, images).
     * @param config Generation configuration.
     * @return std::string The model's response.
     * @throws std::runtime_error If the API request fails or returns an API error.
     */
    std::string generate(const std::vector<ContentPart>& parts, const PromptConfig& config);

private:
    std::string api_key_;
};

#endif // GEMINI_ENGINE_H

