#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "ConfigManager.hpp"
#include "DatabaseManager.hpp"
#include "RedisManager.hpp"
#include "AIAssistant.hpp"
#include "TaskListView.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::make_unique;
using std::unique_ptr;

int main()
{
    try
    {
        // Load configuration
        ConfigManager config;
        if (!config.load_config("config.json"))
        {
            cout << "Warning: Using default configuration settings." << endl;
        }

        // Initialize database
        DatabaseManager db(config.get_database_path());
        db.initilize_database();

        // Initialize Redis (optional)
        unique_ptr<RedisManager> redis = nullptr;
        if (config.is_redis_enabled())
        {
            redis = make_unique<RedisManager>(config.get_redis_host(), config.get_redis_port());
            if (redis->connect())
            {
                cout << "Redis caching enabled." << endl;
            }
            else
            {
                cout << "Warning: Redis connection failed. Continuing without cache." << endl;
                redis = nullptr;
            }
        }

        // Initialize AI Assistant
        AIAssistant ai(config);
        if (ai.is_available())
        {
            cout << "AI features enabled using model: " << config.get_model_name() << endl;
        }
        else
        {
            cout << "AI features are disabled." << endl;
        }

        // Create and run the UI
        TaskListView view(db, ai, redis.get());
        view.run();

        cout << "Thank you for using Teminder!" << endl;
        return 0;
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}
