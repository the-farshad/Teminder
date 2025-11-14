#pragma once

#include <string>
#include <ctime>
#include <optional>
#include <vector>

using std::nullopt;
using std::optional;
using std::string;
using std::time;
using std::time_t;
using std::vector;

/**
 * @brief Represents a single task in the Teminder application.
 *
 * This struct contains all the information needed to manage a task,
 * including its description, completion status, priority, and dates.
 */
struct Task
{
    int id;                    // Primary key (auto-incremented by DB)
    string description;        // Task description
    bool is_completed;         // Completion status
    int priority;              // Task priority
    time_t created_at;         // Creation timestamp
    optional<time_t> due_date; // Optional due date
    vector<string> links;      // Associated links
    vector<int> tags;          // Associated tag IDs
    optional<int> parent_id;   // Optional parent task ID for subtasks
    int progress;              // Task progress (0-100)
    int status;                // Task status: 0=New, 1=In Progress, 2=On Hold, 3=Canceled, 4=Completed

    /**
     * @brief Default Constructor
     */
    Task()
        : id(0), description(""), is_completed(false),
          priority(0), created_at(time(nullptr)), due_date(nullopt), links({}), tags({}), parent_id(nullopt), progress(0), status(0) {}
    /**
     * @brief Constructor with parameters
     */
    Task(int id, const string &desc, bool completed, int prio,
         time_t created, optional<time_t> due = nullopt,
         vector<string> task_links = {}, vector<int> task_tags = {}, optional<int> task_parent_id = nullopt, int task_progress = 0, int task_status = 0)
        : id(id), description(desc), is_completed(completed), priority(prio), created_at(created), due_date(due), links(task_links), tags(task_tags), parent_id(task_parent_id), progress(task_progress), status(task_status) {}
    /**
     * @brief Get a human-readable priority string
     */
    string get_priority_string() const
    {
        switch (priority)
        {
        case 0:
            return "Low";
        case 1:
            return "Medium";
        case 2:
            return "High";
        default:
            return "Unknown";
        }
    }
    /**
     * brief Get a formatted due date string
     */
    string get_due_date_string() const
    {
        if (!due_date.has_value())
        {
            return "No due date";
        }
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M",
                 localtime(&due_date.value()));
        return string(buffer);
    }
    /**
     * @brief Check if task is overdue
     */
    bool is_overdue() const
    {
        if (!due_date.has_value() || is_completed)
        {
            return false;
        }
        return due_date.value() < time(nullptr);
    }

    /**
     * @brief Check if this task is a subtask
     */
    bool is_subtask() const
    {
        return parent_id.has_value();
    }

    /**
     * @brief Get status string
     */
    string get_status_string() const
    {
        switch (status)
        {
        case 0:
            return "New";
        case 1:
            return "In Progress";
        case 2:
            return "On Hold";
        case 3:
            return "Canceled";
        case 4:
            return "Completed";
        default:
            return "Unknown";
        }
    }
};
