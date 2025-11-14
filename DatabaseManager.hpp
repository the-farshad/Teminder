#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <SQLiteCpp/Database.h>
#include "Task.hpp"

using std::optional;
using std::string;
using std::unique_ptr;
using std::vector;

class DatabaseManager
{
public:
    /**
     * @brief Constructor that opens or creates a database.
     * @param db_path The file path to the database (e.g., "tasks.db").
     */
    DatabaseManager(const string &db_path);

    /**
     * @brief Initialize the database schema
     * Creates all necessary tables if they don't exist
     */
    void initilize_database();

    /**
     * @brief Add a new task to the database
     * @param task The task to add
     * @return The ID of the newly created task
     */
    int add_task(const Task &task);

    /**
     * @brief Get all tasks from the database
     * @param include_completed Whether to include completed tasks
     * @return Vector of all tasks
     */
    vector<Task> get_all_tasks(bool include_completed = true);

    /**
     * @brief Get a task by ID
     * @param task_id The ID of the task
     * @return Optional containing the task if found
     */
    optional<Task> get_task_by_id(int task_id);

    /**
     * @brief Update an existing task
     * @param task The task with updated information
     * @return true if successful, false otherwise
     */
    bool update_task(const Task &task);

    /**
     * @brief Delete a task by ID
     * @param task_id The ID of the task to delete
     * @return true if successful, false otherwise
     */
    bool delete_task(int task_id);

    /**
     * @brief Get tasks by priority
     * @param priority The priority level (0=Low, 1=Medium, 2=High)
     * @return Vector of tasks with the specified priority
     */
    vector<Task> get_tasks_by_priority(int priority);

    /**
     * @brief Get overdue tasks
     * @return Vector of overdue tasks
     */
    vector<Task> get_overdue_tasks();

    /**
     * @brief Get subtasks for a parent task
     * @param parent_id The ID of the parent task
     * @return Vector of subtasks
     */
    vector<Task> get_subtasks(int parent_id);

    /**
     * @brief Add a link to a task
     * @param task_id The ID of the task
     * @param link The link URL to add
     * @return true if successful, false otherwise
     */
    bool add_task_link(int task_id, const string &link);

    /**
     * @brief Get all links for a task
     * @param task_id The ID of the task
     * @return Vector of link URLs
     */
    vector<string> get_task_links(int task_id);

private:
    unique_ptr<SQLite::Database> db;
    string db_path;
};