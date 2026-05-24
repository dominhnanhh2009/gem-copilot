cmake_minimum_required(VERSION 3.10)
project(GeminiCopilot)

set(CMAKE_CXX_STANDARD 20)

# Find packages
find_package(CURL REQUIRED)
find_package(jsoncpp REQUIRED)

# Try finding Qt6 first, fallback to Qt5 if needed, or point to CMAKE_PREFIX_PATH if known
find_package(Qt6 COMPONENTS Widgets REQUIRED)

include_directories(include)

add_library(gemini_engine src/gemini_engine.cpp)
target_link_libraries(gemini_engine CURL::libcurl JsonCpp::JsonCpp)

# Main App
# Nạp các file .cpp vào build chính để CMake xử lý MOC đúng cách
# Khi để Q_OBJECT trong file .cpp, CMake cần chỉ định rõ file đó cần được xử lý bởi MOC.
# Cách tốt nhất là dùng set_source_files_properties.

add_executable(screen-copilot screen-copilot.cpp)
set_target_properties(screen-copilot PROPERTIES AUTOMOC ON)
target_link_libraries(screen-copilot Qt6::Widgets gemini_engine)

# Test App
add_executable(test_app test.cpp)
target_link_libraries(test_app gemini_engine Qt6::Widgets)

# Set C++20 for all targets
set_target_properties(screen-copilot PROPERTIES CXX_STANDARD 20)
set_target_properties(test_app PROPERTIES CXX_STANDARD 20)
set_target_properties(gemini_engine PROPERTIES CXX_STANDARD 20)

