#pragma once

#include <string>
#include <vector>
#include "Task.hpp"
#include "ConfigManager.hpp"

using std::string;
using std::vector;

/** AI Assistant interface
 * @brief Handles AI-powered task assistance using Ollama
 * This class provides methods to interact with a local Ollama instance
 * for generating task suggestions, summaries, and breaking down complex tasks.
 */
class AIAssistant
{
public:
    /**
     * @brief Constructor
     * @param config Reference to the application configuration
     */
    explicit AIAssistant(const ConfigManager &config);

    /**
     * @brief Get AI-generated suggestions for next steps on a task
     * @param task The task to analyze
     * @return AI-generated suggestions as a string
     */
    string get_task_suggestions(const Task &task);
    /**
     * @brief Get a summary of the schedule
     * @param tasks Vector of tasks to summarize
     * @return AI-generated summary as a string
     */
    string get_schedule_summary(const vector<Task> &tasks);
    /**
     * @brief Break down a complex task into smaller steps
     * @param task The task to break down
     * @return Vector of smaller task descriptions
     */
    vector<string> break_down_task(const Task &task);
    /**
     * @brief Check if AI features are available
     * @return true if AI is enabled and configured
     */
    bool is_available() const { return ai_enabled; }

private:
    /**
     * @brief Send a request to the Ollama API
     * @param prompt The prompt to send
     * @return The AI response, or error message
     */
    string query_ollama(const string &prompt);
    /**
     * @brief Format tasks into a readable string for the AI
     * @param tasks Vector of tasks to format
     * @return Formatted string representation
     */
    string format_tasks_for_ai(const vector<Task> &tasks);
    const ConfigManager &config;
    bool ai_enabled;
};