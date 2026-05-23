# Gemini-powered Copilot Tool

This project provides a lightweight, C++20-based toolset that acts as an interface to Google's Gemini API, offering functionality similar to GitHub Copilot. It is designed to be a flexible wrapper that ensures compatibility with OpenAI-style API requests while leveraging Gemini as the backend engine.

## Overview
This tool serves as an API wrapper for Gemini. It is intended to be a foundational layer, allowing developers to build various components on top of it without rigid constraints. 

*Note: The project is primarily limited by the available Gemini API token budget.*

## Tech Stack
* **Language:** C++20 (utilizing ISO C++ standards)
* **Environment:** MSYS2 with UCRT64 shell

## Architecture
1.  **API Wrapper Layer:** A foundational layer that translates requests to be OpenAI API-compatible, redirecting them to the Gemini API backend.
2.  **Extensible Components:** The architecture is modular, allowing for flexible development of additional features and components on top of the base layer.
