#pragma once

#include <string>
#include <vector>
#include "Task.hpp"
#include "ConfigManager.hpp"

using std::string;
using std::vector;

/**
 * @brief Google Sheets integration for task export and sync
 * Provides functionality to export tasks to Google Sheets for sharing and backup
 */
class GoogleSheets
{
public:
    /**
     * @brief Constructor
     * @param config Reference to the application configuration
     */
    explicit GoogleSheets(const ConfigManager &config);

    /**
     * @brief Export tasks to a Google Sheet
     * @param tasks Vector of tasks to export
     * @param spreadsheet_id The ID of the target Google Sheet
     * @param sheet_name The name of the sheet/tab (default: "Tasks")
     * @return true if successful, false otherwise
     */
    bool export_tasks(const vector<Task> &tasks,
                      const string &spreadsheet_id,
                      const string &sheet_name = "Tasks");

    /**
     * @brief Create a new Google Sheet with tasks
     * @param tasks Vector of tasks to include
     * @param sheet_title Title for the new spreadsheet
     * @return The spreadsheet ID if successful, empty string otherwise
     */
    string create_sheet_with_tasks(const vector<Task> &tasks,
                                   const string &sheet_title);

    /**
     * @brief Append tasks to an existing sheet
     * @param tasks Vector of tasks to append
     * @param spreadsheet_id The ID of the target Google Sheet
     * @param sheet_name The name of the sheet/tab
     * @return Number of tasks successfully appended
     */
    int append_tasks(const vector<Task> &tasks,
                     const string &spreadsheet_id,
                     const string &sheet_name = "Tasks");

    /**
     * @brief Check if Google Sheets integration is available
     * @return true if enabled and configured, false otherwise
     */
    bool is_available() const { return sheets_enabled && !api_key.empty(); }

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    string get_last_error() const { return last_error; }

private:
    /**
     * @brief Format tasks into rows for Google Sheets
     * @param tasks Vector of tasks to format
     * @return Vector of rows, where each row is a vector of cell values
     */
    vector<vector<string>> format_tasks_for_sheets(const vector<Task> &tasks);

    /**
     * @brief Make an HTTP request to Google Sheets API
     * @param method HTTP method (GET, POST, PUT)
     * @param url API endpoint URL
     * @param json_body Request body (for POST/PUT)
     * @return Response body string
     */
    string make_api_request(const string &method,
                            const string &url,
                            const string &json_body = "");

    const ConfigManager &config;
    string api_key;
    string api_endpoint;
    bool sheets_enabled;
    mutable string last_error;
};
