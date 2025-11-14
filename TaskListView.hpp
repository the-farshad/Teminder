#pragma once

#include <memory>
#include <string>
#include <vector>
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "DatabaseManager.hpp"
#include "AIAssistant.hpp"
#include "RedisManager.hpp"
#include "Task.hpp"

using std::string;
using std::vector;

class TaskListView
{
public:
    TaskListView(DatabaseManager &db_manager, AIAssistant &ai_assistant, RedisManager *redis_manager = nullptr);

    /**
     * @brief Run the main application loop
     */
    void run();

private:
    /**
     * @brief Refresh task list from database
     */
    void refresh_tasks();

    /**
     * @brief Create the main UI layout
     * @return FTXUI component
     */
    ftxui::Component create_ui();

    /**
     * @brief Create the task list display component
     * @return FTXUI component
     */
    ftxui::Component create_task_list();

    /**
     * @brief Create the menu/command bar component
     * @return FTXUI component
     */
    ftxui::Component create_menu();

    /**
     * @brief Render the current UI state
     * @return FTXUI element
     */
    ftxui::Element render();

    /**
     * @brief Add a new task
     */
    void add_task_dialog();

    /**
     * @brief Edit the selected task
     */
    void edit_task_dialog();

    /**
     * @brief Delete the selected task
     */
    void delete_task_dialog();

    /**
     * @brief Confirm deletion of selected task
     */
    void confirm_delete();

    /**
     * @brief Save the task being added or edited
     * @param is_edit true if editing existing task, false if adding new
     */
    void save_task(bool is_edit);

    /**
     * @brief Save a subtask being added
     */
    void save_subtask();

    /**
     * @brief Toggle completion status of selected task
     */
    void toggle_task_completion();

    /**
     * @brief Show AI suggestions for selected task
     */
    void show_ai_suggestions();

    /**
     * @brief Show schedule summary using AI
     */
    void show_schedule_summary();

    /**
     * @brief Export tasks to Google Sheets
     */
    void export_to_sheets();

    /**
     * @brief Add subtask to selected task
     */
    void add_subtask_dialog();

    /**
     * @brief Show settings dialog
     */
    void show_settings();

    /**
     * @brief Sync tasks to Google Sheets
     */
    void sync_to_sheets();

    /**
     * @brief Show help dialog
     */
    void show_help();

    /**
     * @brief Format a task for display
     * @param task The task to format
     * @param is_selected Whether the task is currently selected
     * @return Formatted string
     */
    string format_task(const Task &task, bool is_selected) const;

    // References and pointers
    DatabaseManager &db;
    AIAssistant &ai;
    RedisManager *redis;

    // UI state
    ftxui::ScreenInteractive screen;
    vector<Task> tasks;
    int selected_index;
    bool show_completed;
    string status_message;
    string current_view; // "list", "add", "edit", "help", "ai_suggestions", "delete_confirm"
    bool show_progress;
    int progress_value;
    string progress_message;

    // Input fields
    string input_description;
    int input_priority;
    string input_due_date;
    string input_link;
    int input_progress;
    int current_input_field; // 0=description, 1=due_date, 2=link, 3=progress
};
