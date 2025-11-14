#include "GoogleSheets.hpp"
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <iostream>
#include <sstream>

using cpr::Body;
using cpr::Header;
using cpr::Response;
using cpr::Timeout;
using cpr::Url;
using nlohmann::json;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::stringstream;
using std::to_string;

GoogleSheets::GoogleSheets(const ConfigManager &config)
    : config(config),
      api_key(config.get_google_sheets_api_key()),
      api_endpoint(config.get_google_sheets_endpoint()),
      sheets_enabled(config.is_google_sheets_enabled()),
      last_error("")
{
}

vector<vector<string>> GoogleSheets::format_tasks_for_sheets(const vector<Task> &tasks)
{
    vector<vector<string>> rows;

    // Header row
    rows.push_back({"ID", "Description", "Status", "Priority", "Created At", "Due Date", "Links"});

    // Data rows
    for (const auto &task : tasks)
    {
        vector<string> row;
        row.push_back(to_string(task.id));
        row.push_back(task.description);
        row.push_back(task.is_completed ? "Completed" : "Pending");
        row.push_back(task.get_priority_string());

        // Format created_at
        char created_buffer[80];
        strftime(created_buffer, sizeof(created_buffer), "%Y-%m-%d %H:%M",
                 localtime(&task.created_at));
        row.push_back(string(created_buffer));

        // Format due_date
        row.push_back(task.get_due_date_string());

        // Join links with commas
        stringstream links_ss;
        for (size_t i = 0; i < task.links.size(); ++i)
        {
            if (i > 0)
                links_ss << ", ";
            links_ss << task.links[i];
        }
        row.push_back(links_ss.str());

        rows.push_back(row);
    }

    return rows;
}

string GoogleSheets::make_api_request(const string &method,
                                      const string &url,
                                      const string &json_body)
{
    if (!sheets_enabled)
    {
        last_error = "Google Sheets integration is disabled in configuration";
        return "";
    }

    if (api_key.empty())
    {
        last_error = "Google Sheets API key is not configured";
        return "";
    }

    try
    {
        string full_url = url;
        if (full_url.find('?') != string::npos)
        {
            full_url += "&key=" + api_key;
        }
        else
        {
            full_url += "?key=" + api_key;
        }

        Response response;

        if (method == "GET")
        {
            response = cpr::Get(
                Url{full_url},
                Header{{"Content-Type", "application/json"}},
                Timeout{30000});
        }
        else if (method == "POST")
        {
            response = cpr::Post(
                Url{full_url},
                Header{{"Content-Type", "application/json"}},
                Body{json_body},
                Timeout{30000});
        }
        else if (method == "PUT")
        {
            response = cpr::Put(
                Url{full_url},
                Header{{"Content-Type", "application/json"}},
                Body{json_body},
                Timeout{30000});
        }
        else
        {
            last_error = "Unsupported HTTP method: " + method;
            return "";
        }

        if (response.status_code >= 200 && response.status_code < 300)
        {
            last_error = "";
            return response.text;
        }
        else
        {
            last_error = "API request failed with status " +
                         to_string(response.status_code) + ": " + response.text;
            cerr << last_error << endl;
            return "";
        }
    }
    catch (const exception &e)
    {
        last_error = string("Error making API request: ") + e.what();
        cerr << last_error << endl;
        return "";
    }
}

bool GoogleSheets::export_tasks(const vector<Task> &tasks,
                                const string &spreadsheet_id,
                                const string &sheet_name)
{
    if (!is_available())
    {
        last_error = "Google Sheets integration is not available";
        return false;
    }

    try
    {
        // Format tasks into rows
        auto rows = format_tasks_for_sheets(tasks);

        // Build the values array for the API
        json values_json = json::array();
        for (const auto &row : rows)
        {
            json row_json = json::array();
            for (const auto &cell : row)
            {
                row_json.push_back(cell);
            }
            values_json.push_back(row_json);
        }

        json request_body = {
            {"range", sheet_name + "!A1"},
            {"majorDimension", "ROWS"},
            {"values", values_json}};

        // Clear existing content first
        string clear_url = api_endpoint + "/" + spreadsheet_id +
                           "/values/" + sheet_name + ":clear";

        make_api_request("POST", clear_url, "{}");

        // Now write the new data
        string update_url = api_endpoint + "/" + spreadsheet_id +
                            "/values/" + sheet_name + "!A1?valueInputOption=RAW";

        string response = make_api_request("PUT", update_url, request_body.dump());

        if (response.empty())
        {
            return false;
        }

        cout << "Successfully exported " << tasks.size() << " tasks to Google Sheets" << endl;
        return true;
    }
    catch (const exception &e)
    {
        last_error = string("Error exporting tasks: ") + e.what();
        cerr << last_error << endl;
        return false;
    }
}

string GoogleSheets::create_sheet_with_tasks(const vector<Task> &tasks,
                                             const string &sheet_title)
{
    if (!is_available())
    {
        last_error = "Google Sheets integration is not available";
        return "";
    }

    try
    {
        // Create a new spreadsheet
        json request_body = {
            {"properties", {{"title", sheet_title}}},
            {"sheets", {{{"properties", {{"title", "Tasks"}}}}}}};

        string create_url = api_endpoint;
        string response = make_api_request("POST", create_url, request_body.dump());

        if (response.empty())
        {
            return "";
        }

        // Parse the response to get the spreadsheet ID
        json response_json = json::parse(response);
        string new_spreadsheet_id = response_json["spreadsheetId"].get<string>();

        cout << "Created new spreadsheet with ID: " << new_spreadsheet_id << endl;

        // Export tasks to the new spreadsheet
        if (export_tasks(tasks, new_spreadsheet_id, "Tasks"))
        {
            return new_spreadsheet_id;
        }
        else
        {
            return "";
        }
    }
    catch (const exception &e)
    {
        last_error = string("Error creating spreadsheet: ") + e.what();
        cerr << last_error << endl;
        return "";
    }
}

int GoogleSheets::append_tasks(const vector<Task> &tasks,
                               const string &spreadsheet_id,
                               const string &sheet_name)
{
    if (!is_available())
    {
        last_error = "Google Sheets integration is not available";
        return 0;
    }

    try
    {
        // Format tasks into rows (skip header)
        auto all_rows = format_tasks_for_sheets(tasks);
        vector<vector<string>> data_rows(all_rows.begin() + 1, all_rows.end());

        // Build the values array for the API
        json values_json = json::array();
        for (const auto &row : data_rows)
        {
            json row_json = json::array();
            for (const auto &cell : row)
            {
                row_json.push_back(cell);
            }
            values_json.push_back(row_json);
        }

        json request_body = {
            {"range", sheet_name + "!A2"},
            {"majorDimension", "ROWS"},
            {"values", values_json}};

        string append_url = api_endpoint + "/" + spreadsheet_id +
                            "/values/" + sheet_name + ":append?valueInputOption=RAW";

        string response = make_api_request("POST", append_url, request_body.dump());

        if (response.empty())
        {
            return 0;
        }

        cout << "Successfully appended " << tasks.size() << " tasks to Google Sheets" << endl;
        return tasks.size();
    }
    catch (const exception &e)
    {
        last_error = string("Error appending tasks: ") + e.what();
        cerr << last_error << endl;
        return 0;
    }
}
