cmake_minimum_required(VERSION 3.10)
project(GeminiCopilot)

set(CMAKE_CXX_STANDARD 20)

# Find packages
find_package(CURL REQUIRED)
find_package(jsoncpp REQUIRED)

include_directories(include)

add_library(gemini_engine src/gemini_engine.cpp)
target_link_libraries(gemini_engine CURL::libcurl JsonCpp::JsonCpp)

add_executable(test_app test.cpp)
target_link_libraries(test_app gemini_engine)
