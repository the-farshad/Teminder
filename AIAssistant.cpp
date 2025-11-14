#include "AIAssistant.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <cpr/cpr.h>

using cpr::Body;
using cpr::Header;
using cpr::Timeout;
using cpr::Url;
using nlohmann::json;
using std::cout;
using std::endl;
using std::exception;
using std::istringstream;
using std::string;
using std::stringstream;
using std::to_string;
using std::vector;

AIAssistant::AIAssistant(const ConfigManager &config)
    : config(config), ai_enabled(config.is_ai_enabled())
{
    // Constructor implementation
}

string AIAssistant::query_ollama(const string &prompt)
{
    if (!ai_enabled)
    {
        return "AI features are disabled in configuration.";
    }
    try
    {
        // Prepare the JSON request for Ollama API
        json request_body = {
            {"model", config.get_model_name()},
            {"prompt", prompt},
            {"stream", false},
            {"options", {{"temperature", config.get_temperature()}, {"num_predict", config.get_max_tokens()}}}};

        // Make the HTTP POST request to Ollama
        string endpoint = config.get_ollama_endpoint() + "/api/generate";

        auto response = cpr::Post(
            Url{endpoint},
            Header{{"Content-Type", "application/json"}},
            Body{request_body.dump()},
            Timeout{30000} // 30 second timeout
        );

        if (response.status_code != 200)
        {
            return "Error: Unable to reach Ollama API (status " +
                   to_string(response.status_code) + "). " +
                   "Make sure Ollama is running on " + config.get_ollama_endpoint();
        }

        // Parse the response
        auto response_json = json::parse(response.text);

        if (response_json.contains("response"))
        {
            return response_json["response"].get<string>();
        }
        else
        {
            return "Error: Unexpected response format from Ollama API.";
        }
    }
    catch (exception &e)
    {
        return string("Error querying Ollama: ") + e.what();
    }
}

string AIAssistant::get_task_suggestions(const Task &task)
{
    if (!ai_enabled)
    {
        return "AI task suggestions are disabled. Enable AI in config.json to use this feature.";
    }
    stringstream prompt;
    prompt << config.get_task_suggestion_prompt() << "\n\n";
    prompt << "Task: " << task.description << "\n";
    prompt << "Priority: " << task.get_priority_string() << "\n";

    if (task.due_date.has_value())
    {
        prompt << "Due Date: " << task.get_due_date_string() << "\n";
    }

    if (task.is_overdue())
    {
        prompt << "Status: OVERDUE!\n";
    }

    prompt << "\nPlease suggest specific, actionable next steps:";

    return query_ollama(prompt.str());
}

string AIAssistant::get_schedule_summary(const vector<Task> &tasks)
{
    if (!ai_enabled)
    {
        return "AI schedule summary is disabled. Enable AI in config.json to use this feature.";
    }

    stringstream prompt;
    prompt << config.get_summary_prompt() << "\n\n";
    prompt << "Here are the tasks to summarize:\n\n";
    prompt << format_tasks_for_ai(tasks);
    prompt << "\nPlease provide a concise summary and recommendations:";
    return query_ollama(prompt.str());
}

vector<string> AIAssistant::break_down_task(const Task &task)
{
    if (!ai_enabled)
    {
        return {"AI task breakdown is disabled. Enable AI in config.json to use this feature."};
    }

    stringstream prompt;
    prompt << "You are a task management assistant. Break down the following task into "
           << "3-5 smaller, actionable sub-tasks. Format your response as a numbered list.\n\n";
    prompt << "Task: " << task.description << "\n";
    prompt << "Priority: " << task.get_priority_string() << "\n\n";
    prompt << "Sub-tasks:";

    string response = query_ollama(prompt.str());

    // Parse the response into individual sub-tasks
    vector<string> subtasks;
    istringstream stream(response);
    string line;

    while (getline(stream, line))
    {
        // Remove leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t\n\r");
        size_t end = line.find_last_not_of(" \t\n\r");

        if (start != string::npos && end != string::npos)
        {
            line = line.substr(start, end - start + 1);

            // Skip empty lines
            if (!line.empty())
            {
                // Remove common list prefixes (1., -, *, etc.)
                if (line.length() > 2 &&
                    (isdigit(line[0]) || line[0] == '-' || line[0] == '*'))
                {
                    size_t content_start = line.find_first_not_of("0123456789.-* \t", 0);
                    if (content_start != string::npos)
                    {
                        line = line.substr(content_start);
                    }
                }

                if (!line.empty())
                {
                    subtasks.push_back(line);
                }
            }
        }
    }

    return subtasks;
}

string AIAssistant::format_tasks_for_ai(const vector<Task> &tasks)
{
    stringstream formatted;
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        const auto &task = tasks[i];
        formatted << i + 1 << ". " << task.description
                  << " [Priority: " << task.get_priority_string() << "]";

        if (task.due_date.has_value())
        {
            formatted << " [Due: " << task.get_due_date_string() << "]";
        }

        if (task.is_completed)
        {
            formatted << " [Status: Completed]";
        }
        else if (task.is_overdue())
        {
            formatted << " [Status: OVERDUE]";
        }

        formatted << "\n";
    }
    return formatted.str();
}