#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>

using nlohmann::json;

using std::cerr;
using std::endl;
using std::exception;
using std::ifstream;
using std::string;

bool ConfigManager::load_config(const string &config_path)
{
    try
    {
        ifstream config_file(config_path);

        if (!config_file.is_open())
        {
            cerr << "Warning: Could not open config file '" << config_path
                 << "'. Using default setting." << endl;
            return false;
        }

        json config_json;
        config_file >> config_json;

        // Load AI settings
        if (config_json.contains("ai"))
        {
            auto ai_config = config_json["ai"];

            if (ai_config.contains("enabled"))
            {
                ai_enabled = ai_config["enabled"].get<bool>();
            }

            if (ai_config.contains("ollama_endpoint"))
            {
                ollama_endpoint = ai_config["ollama_endpoint"].get<string>();
            }
            if (ai_config.contains("model_name"))
            {
                model_name = ai_config["model_name"].get<string>();
            }
            if (ai_config.contains("max_tokens"))
            {
                max_tokens = ai_config["max_tokens"].get<int>();
            }
            if (ai_config.contains("temperature"))
            {
                temperature = ai_config["temperature"].get<float>();
            }
        }

        // Load custom prompts
        if (config_json.contains("prompts"))
        {
            auto prompts_config = config_json["prompts"];
            if (prompts_config.contains("task_suggestion"))
            {
                task_suggestion_prompt = prompts_config["task_suggestion"].get<string>();
            }

            if (prompts_config.contains("summary"))
            {
                summary_prompt = prompts_config["summary"].get<string>();
            }

            // Load database settings
            if (config_json.contains("database"))
            {
                auto db_config = config_json["database"];
                if (db_config.contains("path"))
                {
                    database_path = db_config["path"].get<string>();
                }
            }
        }

        // Load redis settings
        if (config_json.contains("redis"))
        {
            auto redis_config = config_json["redis"];

            if (redis_config.contains("enabled"))
            {
                redis_enabled = redis_config["enabled"].get<bool>();
            }
            if (redis_config.contains("host"))
            {
                redis_host = redis_config["host"].get<string>();
            }
            if (redis_config.contains("port"))
            {
                redis_port = redis_config["port"].get<int>();
            }
        }

        // Google sheets settings
        if (config_json.contains("google_sheets"))
        {
            auto google_sheets_config = config_json["google_sheets"];

            if (google_sheets_config.contains("enabled"))
            {
                google_sheets_enabled = google_sheets_config["enabled"].get<bool>();
            }

            if (google_sheets_config.contains("api_key"))
            {
                google_sheets_api_key = google_sheets_config["api_key"].get<string>();
            }

            if (google_sheets_config.contains("endpoint"))
            {
                google_sheets_endpoint = google_sheets_config["endpoint"].get<string>();
            }
        }

        return true;
    }
    catch (const exception &e)
    {
        cerr << "Error loading config: " << e.what() << endl;
        cerr << "Using default settings." << endl;
        return false;
    }
}