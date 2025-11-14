#ifndef ConfigManager_H
#define ConfigManager_H

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using std::string;

/**
 * @brief Manage application configuration from config.json
 * Handles loading and parsing configuration settings for the Teminder application,
 * includeing AI models seetings, API endpoints, and application preferences.
 */

class ConfigManager
{
public:
    /**
     * @brief Load configuration from a JSON file
     * @param config_path Path to the config.json file
     * @return true if successful, false otherwise
     */
    bool load_config(const string &config_path);

    /**
     * @brief Get the Ollama API endpoint
     * @return The API endpoint URL
     */
    string get_ollama_endpoint() const { return ollama_endpoint; }

    /**
     * @brief Get the AI model name to use
     * @return The AI model name (e.g., "llama3", "mistral")
     */
    string get_model_name() const { return model_name; }

    /**
     * @brief Get the task suggestion prompt
     * @return The task suggestion prompt
     */
    string get_task_suggestion_prompt() const { return task_suggestion_prompt; }

    /**
     * @brief Get the summary prompt
     * @return The summary prompt
     */
    string get_summary_prompt() const { return summary_prompt; }

    /**
     * @brief Get the database file path
     * @return The database file path
     */
    string get_database_path() const { return database_path; }

    /**
     * @brief Check if AI is enabled
     * @return true if AI is enabled, false otherwise
     */
    bool is_ai_enabled() const { return ai_enabled; }

    /**
     * @brief Load configuration from a JSON file
     * @return Temperature value
     */
    int get_max_tokens() const { return max_tokens; }

    /**
     * @brief Load temperature setting
     * @return Temperature value
     */
    float get_temperature() const { return temperature; }

    /**
     * @brief Check if Redis is enabled
     * @return true if Redis is enabled, false otherwise
     */
    bool is_redis_enabled() const { return redis_enabled; }

    /**
     * @brief Get Redis host
     * @return Redis host
     */
    string get_redis_host() const { return redis_host; }

    /**
     * @brief Get Redis port
     * @return Redis port
     */
    int get_redis_port() const { return redis_port; }

    /**
     * @brief Check if Google Sheets integration is enabled
     * @return true if Google Sheets is enabled, false otherwise
     */
    bool is_google_sheets_enabled() const { return google_sheets_enabled; }

    /**
     * @brief Get Google Sheets API key
     * @return Google Sheets API key
     */
    string get_google_sheets_api_key() const { return google_sheets_api_key; }

    /**
     * @brief Get Google Sheets API endpoint
     * @return Google Sheets API endpoint
     */
    string get_google_sheets_endpoint() const { return google_sheets_endpoint; }

private:
    // AI settings
    string ollama_endpoint = "http://localhost:11434";
    string model_name = "phi4:latest";
    bool ai_enabled = true;
    int max_tokens = 1000;
    float temperature = 0.7f;

    // Custom prompts
    string task_suggestion_prompt =
        "You are a helpful task management assistant. "
        "Analyze the following task and suggest the next actionable steps to complete it. "
        "Be concise and practical.";

    string summary_prompt =
        "You are a helpful task management assistant. "
        "Summarize the following tasks and provide a brief overview of what needs to be done. "
        "Highlight any overdue or high-priority items.";

    // Database settings
    string database_path = "tasks.db";

    // Redin settings
    bool redis_enabled = false;
    string redis_host = "localhost";
    int redis_port = 6379;

    // Google sheets settings
    bool google_sheets_enabled = false;
    string google_sheets_endpoint = "https://sheets.googleapis.com/v4/spreadsheets";
    string google_sheets_api_key = "";
};

#endif