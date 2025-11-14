#include "TaskListView.hpp"
#include "GoogleSheets.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

using ftxui::CatchEvent;
using ftxui::Component;
using ftxui::Element;
using ftxui::Elements;
using ftxui::Event;
using ftxui::Renderer;
using ftxui::ScreenInteractive;
using std::cout;
using std::endl;
using std::stringstream;
using std::to_string;

TaskListView::TaskListView(DatabaseManager &db_manager, AIAssistant &ai_assistant, RedisManager *redis_manager)
    : db(db_manager), ai(ai_assistant), redis(redis_manager),
      screen(ScreenInteractive::Fullscreen()),
      selected_index(0), show_completed(true),
      status_message("Welcome to Teminder!"),
      current_view("list"),
      show_progress(false), progress_value(0), progress_message(""),
      input_description(""), input_priority(1), input_due_date(""), input_link(""), input_progress(0),
      current_input_field(0)
{
    refresh_tasks();
}

void TaskListView::refresh_tasks()
{
    tasks = db.get_all_tasks(show_completed);
    if (selected_index >= static_cast<int>(tasks.size()))
    {
        selected_index = tasks.size() > 0 ? tasks.size() - 1 : 0;
    }
}

string TaskListView::format_task(const Task &task, bool is_selected) const
{
    stringstream ss;

    // Checkbox
    ss << (task.is_completed ? "[✓] " : "[ ] ");

    // Priority indicator
    if (task.priority == 2)
        ss << "🔴 ";
    else if (task.priority == 1)
        ss << "🟡 ";
    else
        ss << "🟢 ";

    // Description
    ss << task.description;

    // Due date
    if (task.due_date.has_value())
    {
        ss << " (Due: " << task.get_due_date_string();
        if (task.is_overdue())
        {
            ss << " - OVERDUE!";
        }
        ss << ")";
    }

    // Links indicator
    if (!task.links.empty())
    {
        ss << " 🔗" << task.links.size();
    }

    // Subtask indicator
    if (task.is_subtask())
    {
        ss << " [subtask]";
    }

    // Get subtask count for this task
    auto subtasks = db.get_subtasks(task.id);
    if (!subtasks.empty())
    {
        int completed_subtasks = 0;
        for (const auto &st : subtasks)
        {
            if (st.is_completed)
                completed_subtasks++;
        }
        ss << " [" << completed_subtasks << "/" << subtasks.size() << " subtasks]";
    }

    // Progress bar
    if (task.progress > 0)
    {
        ss << " [" << task.progress << "%]";
    }

    return ss.str();
}

Component TaskListView::create_task_list()
{
    return Renderer([&]
                    {
        ftxui::Elements task_elements;

        if (tasks.empty())
        {
            task_elements.push_back(ftxui::text("No tasks found. Press 'a' to add a new task.") | ftxui::center);
        }
        else
        {
            for (size_t i = 0; i < tasks.size(); ++i)
            {
                const auto &task = tasks[i];
                bool is_selected = (static_cast<int>(i) == selected_index);

                auto task_text = ftxui::text(format_task(task, is_selected));

                if (is_selected)
                {
                    task_elements.push_back(task_text | ftxui::inverted | ftxui::bold);
                }
                else if (task.is_overdue())
                {
                    task_elements.push_back(task_text | ftxui::color(ftxui::Color::Red));
                }
                else if (task.is_completed)
                {
                    task_elements.push_back(task_text | ftxui::dim);
                }
                else
                {
                    task_elements.push_back(task_text);
                }
            }
        }

        return ftxui::vbox(task_elements) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex; });
}

Component TaskListView::create_menu()
{
    return Renderer([&]
                    { return ftxui::hbox({
                                 ftxui::text("Commands: ") | ftxui::bold,
                                 ftxui::text("[a]dd "),
                                 ftxui::text("[t]subtask "),
                                 ftxui::text("[e]dit "),
                                 ftxui::text("[d]elete "),
                                 ftxui::text("[Space]toggle "),
                                 ftxui::text("[c]ompleted "),
                                 ftxui::text("[g]settings "),
                                 ftxui::text("[G]sync "),
                                 ftxui::text("[h]elp "),
                                 ftxui::text("[q]uit "),
                             }) |
                             ftxui::border; });
}

Element TaskListView::render()
{
    // Header
    auto header = ftxui::hbox({
                      ftxui::text("Teminder - Task Manager") | ftxui::bold | ftxui::center,
                  }) |
                  ftxui::border;

    // Status bar with overall progress
    int total_tasks = tasks.size();
    int completed_tasks = 0;
    for (const auto &task : tasks)
    {
        if (task.is_completed)
            completed_tasks++;
    }

    float completion_percentage = total_tasks > 0 ? (float)completed_tasks / total_tasks : 0.0f;

    auto status_bar = ftxui::vbox({
                          ftxui::hbox({
                              ftxui::text(status_message),
                              ftxui::separator(),
                              ftxui::text(" Tasks: " + to_string(tasks.size())),
                              ftxui::separator(),
                              ftxui::text(show_completed ? " [All]" : " [Active]"),
                          }),
                          ftxui::hbox({
                              ftxui::text("Progress: "),
                              ftxui::gauge(completion_percentage) | ftxui::flex,
                              ftxui::text(" " + to_string(completed_tasks) + "/" + to_string(total_tasks) + " completed"),
                          }),
                      }) |
                      ftxui::border;

    // Main content
    Element content;

    if (current_view == "list")
    {
        content = ftxui::vbox({
            header,
            create_task_list()->Render(),
            create_menu()->Render(),
            status_bar,
        });
    }
    else if (current_view == "help")
    {
        content = ftxui::vbox({
            header,
            ftxui::vbox({
                ftxui::text("Teminder Help") | ftxui::bold | ftxui::center,
                ftxui::text(""),
                ftxui::text("Main View Shortcuts:"),
                ftxui::text("  a - Add new task"),
                ftxui::text("  t - Add subtask to selected task ★"),
                ftxui::text("  e - Edit selected task"),
                ftxui::text("  d - Delete selected task (with confirmation)"),
                ftxui::text("  Space - Toggle task completion"),
                ftxui::text("  c - Toggle show/hide completed tasks"),
                ftxui::text("  s - Get AI suggestions for selected task"),
                ftxui::text("  S - Get AI schedule summary"),
                ftxui::text("  g - Open settings"),
                ftxui::text("  G - Sync tasks to Google Sheets ★"),
                ftxui::text("  h - Show this help"),
                ftxui::text("  q - Quit application"),
                ftxui::text("  ↑/↓ - Navigate tasks"),
                ftxui::text(""),
                ftxui::text("In Add/Edit Dialog:"),
                ftxui::text("  Tab - Switch between fields (Description/Date/Link/Progress)"),
                ftxui::text("  Type - Edit active field (highlighted in yellow)"),
                ftxui::text("  +/- - Adjust priority or progress"),
                ftxui::text("  Backspace - Delete character"),
                ftxui::text("  Enter - Save changes"),
                ftxui::text("  ESC - Cancel"),
                ftxui::text(""),
                ftxui::text("Press any key to return..."),
            }) | ftxui::border |
                ftxui::center,
            status_bar,
        });
    }
    else if (current_view == "ai_suggestions")
    {
        content = ftxui::vbox({
            header,
            ftxui::vbox({
                ftxui::text("AI Suggestions") | ftxui::bold | ftxui::center,
                ftxui::text(""),
                ftxui::text(status_message) | ftxui::flex,
                ftxui::text(""),
                ftxui::text("Press any key to return..."),
            }) | ftxui::border |
                ftxui::flex,
            status_bar,
        });
    }
    else if (current_view == "add" || current_view == "edit")
    {
        bool is_edit = (current_view == "edit");
        string title = is_edit ? "Edit Task" : "Add New Task";

        // Highlight active field
        auto desc_style = (current_input_field == 0) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;
        auto date_style = (current_input_field == 1) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;
        auto link_style = (current_input_field == 2) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;
        auto progress_style = (current_input_field == 3) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;

        content = ftxui::vbox({
            header,
            ftxui::vbox({
                ftxui::text(title) | ftxui::bold | ftxui::center,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Description]: "), ftxui::text(input_description.empty() ? "_" : input_description) | ftxui::flex}) | desc_style,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("Priority: "), ftxui::text(to_string(input_priority) + " (" + (input_priority == 0 ? "Low" : input_priority == 1 ? "Medium"
                                                                                                                                                          : "High") +
                                                                    ")") |
                                                            ftxui::flex}),
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Due Date]: "), ftxui::text(input_due_date.empty() ? "YYYY-MM-DD HH:MM" : input_due_date) | ftxui::flex}) | date_style,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Link]: "), ftxui::text(input_link.empty() ? "https://..." : input_link) | ftxui::flex}) | link_style,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Progress]: "), ftxui::text(to_string(input_progress) + "%") | ftxui::flex}) | progress_style,
                ftxui::separator(),
                ftxui::text(""),
                ftxui::text("Controls:"),
                ftxui::text("  Tab - Switch between fields"),
                ftxui::text("  Type - Edit active field (yellow)"),
                ftxui::text("  +/- - Change priority or progress"),
                ftxui::text("  Backspace - Delete character"),
                ftxui::text("  Enter - Save, ESC - Cancel"),
            }) | ftxui::border |
                ftxui::center,
            status_bar,
        });
    }
    else if (current_view == "delete_confirm")
    {
        content = ftxui::vbox({
            header,
            ftxui::vbox({
                ftxui::text("Delete Task") | ftxui::bold | ftxui::center | ftxui::color(ftxui::Color::Red),
                ftxui::text(""),
                ftxui::text(status_message) | ftxui::center,
                ftxui::text(""),
                ftxui::text("Press 'y' to confirm, any other key to cancel") | ftxui::center,
            }) | ftxui::border |
                ftxui::center,
            status_bar,
        });
    }
    else if (current_view == "add_subtask")
    {
        // Subtask dialog - same as add/edit but shows parent task
        auto desc_style = (current_input_field == 0) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;
        auto date_style = (current_input_field == 1) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;
        auto link_style = (current_input_field == 2) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;
        auto progress_style = (current_input_field == 3) ? ftxui::bold | ftxui::color(ftxui::Color::Yellow) : ftxui::nothing;

        content = ftxui::vbox({
            header,
            ftxui::vbox({
                ftxui::text("Add Subtask") | ftxui::bold | ftxui::center,
                ftxui::text(status_message) | ftxui::center,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Description]: "), ftxui::text(input_description.empty() ? "_" : input_description) | ftxui::flex}) | desc_style,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("Priority: "), ftxui::text(to_string(input_priority) + " (" + (input_priority == 0 ? "Low" : input_priority == 1 ? "Medium"
                                                                                                                                                          : "High") +
                                                                    ")") |
                                                            ftxui::flex}),
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Due Date]: "), ftxui::text(input_due_date.empty() ? "YYYY-MM-DD HH:MM" : input_due_date) | ftxui::flex}) | date_style,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Link]: "), ftxui::text(input_link.empty() ? "https://..." : input_link) | ftxui::flex}) | link_style,
                ftxui::separator(),
                ftxui::hbox({ftxui::text("[Progress]: "), ftxui::text(to_string(input_progress) + "%") | ftxui::flex}) | progress_style,
                ftxui::separator(),
                ftxui::text("Tab - Switch | +/- - Priority/Progress | Enter - Save | ESC - Cancel"),
            }) | ftxui::border |
                ftxui::center,
            status_bar,
        });
    }
    else if (current_view == "settings")
    {
        content = ftxui::vbox({
            header,
            ftxui::vbox({
                ftxui::text("Settings") | ftxui::bold | ftxui::center,
                ftxui::text(""),
                ftxui::hbox({ftxui::text("Show Completed Tasks: "), ftxui::text(show_completed ? "ON" : "OFF") | ftxui::bold}),
                ftxui::text(""),
                ftxui::text("Controls:"),
                ftxui::text("  c - Toggle show completed tasks"),
                ftxui::text("  ESC - Close settings"),
                ftxui::text(""),
                ftxui::text(status_message),
            }) | ftxui::border |
                ftxui::center,
            status_bar,
        });
    }

    // Show progress bar if active
    if (show_progress)
    {
        auto progress_bar = ftxui::hbox({
                                ftxui::text(progress_message + " "),
                                ftxui::gauge(progress_value / 100.0f) | ftxui::flex,
                                ftxui::text(" " + to_string(progress_value) + "%"),
                            }) |
                            ftxui::border;

        content = ftxui::vbox({
            content | ftxui::flex,
            progress_bar,
        });
    }

    return content;
}

void TaskListView::add_task_dialog()
{
    // Reset input fields
    input_description = "";
    input_priority = 1;
    input_due_date = "";
    input_link = "";
    input_progress = 0;
    current_input_field = 0;

    current_view = "add";
    status_message = "Enter task details (Tab to switch fields, ESC to cancel, Enter to save)";
}
void TaskListView::toggle_task_completion()
{
    if (tasks.empty() || selected_index >= static_cast<int>(tasks.size()))
    {
        status_message = "No task selected.";
        return;
    }

    Task &task = tasks[selected_index];
    task.is_completed = !task.is_completed;

    if (db.update_task(task))
    {
        status_message = task.is_completed ? "Task marked as completed!" : "Task marked as pending!";

        // Update cache
        if (redis && redis->is_connected())
        {
            redis->cache_task(task);
        }

        refresh_tasks();

        // Force UI refresh to update display immediately
        screen.PostEvent(Event::Custom);
    }
    else
    {
        status_message = "Failed to update task.";
    }
}

void TaskListView::delete_task_dialog()
{
    if (tasks.empty() || selected_index >= static_cast<int>(tasks.size()))
    {
        status_message = "No task selected.";
        return;
    }

    current_view = "delete_confirm";
    const Task &task = tasks[selected_index];
    status_message = "Delete task: \"" + task.description + "\"? (y/N)";
}

void TaskListView::confirm_delete()
{
    if (tasks.empty() || selected_index >= static_cast<int>(tasks.size()))
    {
        current_view = "list";
        status_message = "No task selected.";
        return;
    }

    const Task &task = tasks[selected_index];

    show_progress = true;
    progress_value = 0;
    progress_message = "Deleting task...";
    screen.PostEvent(Event::Custom);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (db.delete_task(task.id))
    {
        progress_value = 50;
        screen.PostEvent(Event::Custom);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Remove from cache
        if (redis && redis->is_connected())
        {
            redis->invalidate_task(task.id);
        }

        progress_value = 100;
        screen.PostEvent(Event::Custom);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        status_message = "Task deleted successfully!";
        refresh_tasks();

        // Force UI refresh
        screen.PostEvent(Event::Custom);
    }
    else
    {
        status_message = "Failed to delete task.";
    }

    show_progress = false;
    current_view = "list";

    // Final screen refresh
    screen.PostEvent(Event::Custom);
}

void TaskListView::save_task(bool is_edit)
{
    if (input_description.empty())
    {
        status_message = "Task description cannot be empty!";
        return;
    }

    show_progress = true;
    progress_value = 0;
    progress_message = is_edit ? "Updating task..." : "Creating task...";
    screen.PostEvent(Event::Custom);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Task task;
    if (is_edit && selected_index < static_cast<int>(tasks.size()))
    {
        task = tasks[selected_index];
    }
    else
    {
        task.created_at = time(nullptr);
        task.is_completed = false;
    }

    task.description = input_description;
    task.priority = input_priority;
    task.progress = input_progress;

    // Parse due date (accepts multiple formats)
    // Formats: "2025-11-20", "2025-11-20 14:30", "2025-11-20 14:30:00"
    if (!input_due_date.empty())
    {
        struct tm tm = {};
        int hour = 0, minute = 0, second = 0;

        // Try parsing YYYY-MM-DD HH:MM:SS format
        if (sscanf(input_due_date.c_str(), "%d-%d-%d %d:%d:%d",
                   &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &hour, &minute, &second) >= 3)
        {
            tm.tm_year -= 1900; // Years since 1900
            tm.tm_mon -= 1;     // Months since January (0-11)
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = second;
            task.due_date = mktime(&tm);
        }
        // Try parsing YYYY-MM-DD HH:MM format
        else if (sscanf(input_due_date.c_str(), "%d-%d-%d %d:%d",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &hour, &minute) >= 3)
        {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = 0;
            task.due_date = mktime(&tm);
        }
        // Try parsing just YYYY-MM-DD format (default to midnight)
        else if (sscanf(input_due_date.c_str(), "%d-%d-%d",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday) >= 3)
        {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            task.due_date = mktime(&tm);
        }
        else
        {
            // Invalid format - show error
            status_message = "Invalid date format! Use: YYYY-MM-DD or YYYY-MM-DD HH:MM";
            show_progress = false;
            return;
        }
    }
    else
    {
        task.due_date = std::nullopt;
    }

    progress_value = 30;
    screen.PostEvent(Event::Custom);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int result;
    if (is_edit)
    {
        if (db.update_task(task))
        {
            result = task.id;
        }
        else
        {
            result = -1;
        }
    }
    else
    {
        result = db.add_task(task);
        task.id = result;
    }

    progress_value = 70;
    screen.PostEvent(Event::Custom);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (result > 0)
    {
        // Add link if provided
        if (!input_link.empty())
        {
            db.add_task_link(task.id, input_link);
        }

        // Cache in Redis if available
        if (redis && redis->is_connected())
        {
            redis->cache_task(task);
        }

        progress_value = 100;
        screen.PostEvent(Event::Custom);

        status_message = is_edit ? "Task updated successfully!" : "Task added successfully!";
        refresh_tasks();

        // Force UI refresh to update overdue status
        screen.PostEvent(Event::Custom);
    }
    else
    {
        status_message = is_edit ? "Failed to update task." : "Failed to add task.";
    }

    show_progress = false;
    current_view = "list";

    // Final screen refresh to ensure overdue status is displayed
    screen.PostEvent(Event::Custom);
}

void TaskListView::save_subtask()
{
    if (input_description.empty())
    {
        status_message = "Subtask description cannot be empty!";
        return;
    }

    if (tasks.empty() || selected_index >= static_cast<int>(tasks.size()))
    {
        status_message = "No parent task selected.";
        return;
    }

    show_progress = true;
    progress_value = 0;
    progress_message = "Creating subtask...";
    screen.PostEvent(Event::Custom);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Task subtask;
    subtask.created_at = time(nullptr);
    subtask.is_completed = false;
    subtask.description = input_description;
    subtask.priority = input_priority;
    subtask.progress = input_progress;
    subtask.parent_id = tasks[selected_index].id; // Set parent

    // Parse due date (same logic as save_task)
    if (!input_due_date.empty())
    {
        struct tm tm = {};
        int hour = 0, minute = 0, second = 0;

        if (sscanf(input_due_date.c_str(), "%d-%d-%d %d:%d:%d",
                   &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &hour, &minute, &second) >= 3)
        {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = second;
            subtask.due_date = mktime(&tm);
        }
        else if (sscanf(input_due_date.c_str(), "%d-%d-%d %d:%d",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &hour, &minute) >= 3)
        {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = 0;
            subtask.due_date = mktime(&tm);
        }
        else if (sscanf(input_due_date.c_str(), "%d-%d-%d",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday) >= 3)
        {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            subtask.due_date = mktime(&tm);
        }
        else
        {
            status_message = "Invalid date format! Use: YYYY-MM-DD or YYYY-MM-DD HH:MM";
            show_progress = false;
            return;
        }
    }
    else
    {
        subtask.due_date = std::nullopt;
    }

    progress_value = 30;
    screen.PostEvent(Event::Custom);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int result = db.add_task(subtask);
    subtask.id = result;

    progress_value = 70;
    screen.PostEvent(Event::Custom);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (result > 0)
    {
        // Add link if provided
        if (!input_link.empty())
        {
            db.add_task_link(subtask.id, input_link);
        }

        // Cache in Redis if available
        if (redis && redis->is_connected())
        {
            redis->cache_task(subtask);
        }

        progress_value = 100;
        screen.PostEvent(Event::Custom);

        status_message = "Subtask added successfully!";
        refresh_tasks();
        screen.PostEvent(Event::Custom);
    }
    else
    {
        status_message = "Failed to add subtask.";
    }

    show_progress = false;
    current_view = "list";
    screen.PostEvent(Event::Custom);
}

void TaskListView::show_ai_suggestions()
{
    if (tasks.empty() || selected_index >= static_cast<int>(tasks.size()))
    {
        status_message = "No task selected.";
        return;
    }

    if (!ai.is_available())
    {
        status_message = "AI features are not available. Check your configuration.";
        return;
    }

    current_view = "ai_suggestions";
    const Task &task = tasks[selected_index];

    status_message = "Getting AI suggestions...\n\n";
    screen.PostEvent(Event::Custom); // Force refresh

    string suggestions = ai.get_task_suggestions(task);
    status_message = "AI Suggestions for: " + task.description + "\n\n" + suggestions;
}

void TaskListView::show_schedule_summary()
{
    if (!ai.is_available())
    {
        status_message = "AI features are not available. Check your configuration.";
        return;
    }

    current_view = "ai_suggestions";
    status_message = "Generating schedule summary...\n\n";
    screen.PostEvent(Event::Custom); // Force refresh

    string summary = ai.get_schedule_summary(tasks);
    status_message = "Schedule Summary:\n\n" + summary;
}

void TaskListView::show_help()
{
    current_view = "help";
}

void TaskListView::run()
{
    auto main_container = ftxui::Container::Vertical({});

    auto component = CatchEvent(main_container, [&](Event event)
                                {
        // Handle delete confirmation
        if (current_view == "delete_confirm")
        {
            if (event == Event::Character('y') || event == Event::Character('Y'))
            {
                confirm_delete();
            }
            else
            {
                current_view = "list";
                status_message = "Delete cancelled.";
            }
            return true;
        }
        
        // Handle add/edit/add_subtask dialogs
        if (current_view == "add" || current_view == "edit" || current_view == "add_subtask")
        {
            bool is_edit = (current_view == "edit");
            bool is_subtask = (current_view == "add_subtask");
            
            if (event == Event::Escape)
            {
                current_view = "list";
                status_message = is_edit ? "Edit cancelled." : is_subtask ? "Subtask cancelled." : "Add cancelled.";
                return true;
            }
            else if (event == Event::Return)
            {
                if (is_subtask)
                {
                    save_subtask();
                }
                else
                {
                    save_task(is_edit);
                }
                return true;
            }
            else if (event == Event::Tab)
            {
                // Cycle through fields: description -> due_date -> link -> progress -> description
                current_input_field = (current_input_field + 1) % 4;
                return true;
            }
            else if (event == Event::Backspace)
            {
                // Delete from active field
                if (current_input_field == 0 && !input_description.empty())
                {
                    input_description.pop_back();
                }
                else if (current_input_field == 1 && !input_due_date.empty())
                {
                    input_due_date.pop_back();
                }
                else if (current_input_field == 2 && !input_link.empty())
                {
                    input_link.pop_back();
                }
                // Progress field doesn't need backspace (numeric only)
                return true;
            }
            else if (event == Event::Character('+'))
            {
                // Change priority when in description field, progress when in progress field
                if (current_input_field == 0 && input_priority < 2)
                {
                    input_priority++;
                }
                else if (current_input_field == 3 && input_progress < 100)
                {
                    input_progress += 5; // Increment by 5%
                    if (input_progress > 100) input_progress = 100;
                }
                else
                {
                    // Add + to active field
                    if (current_input_field == 0)
                    {
                        input_description += '+';
                    }
                    else if (current_input_field == 1)
                    {
                        input_due_date += '+';
                    }
                    else if (current_input_field == 2)
                    {
                        input_link += '+';
                    }
                }
                return true;
            }
            else if (event == Event::Character('-'))
            {
                // Change priority when in description field, progress when in progress field
                if (current_input_field == 0 && input_priority > 0)
                {
                    input_priority--;
                }
                else if (current_input_field == 3 && input_progress > 0)
                {
                    input_progress -= 5; // Decrement by 5%
                    if (input_progress < 0) input_progress = 0;
                }
                else
                {
                    // Add - to active field (needed for dates like 2025-11-20)
                    if (current_input_field == 0)
                    {
                        input_description += '-';
                    }
                    else if (current_input_field == 1)
                    {
                        input_due_date += '-';
                    }
                    else if (current_input_field == 2)
                    {
                        input_link += '-';
                    }
                }
                return true;
            }
            else if (event.is_character())
            {
                // Add to active field
                if (current_input_field == 0)
                {
                    input_description += event.character();
                }
                else if (current_input_field == 1)
                {
                    input_due_date += event.character();
                }
                else if (current_input_field == 2)
                {
                    input_link += event.character();
                }
                else if (current_input_field == 3)
                {
                    // Progress field: only accept digits
                    string char_str = event.character();
                    if (!char_str.empty() && char_str[0] >= '0' && char_str[0] <= '9')
                    {
                        string temp = to_string(input_progress) + char_str;
                        int val = std::stoi(temp);
                        if (val <= 100)
                        {
                            input_progress = val;
                        }
                    }
                }
                return true;
            }
            return true;
        }
        
        // Handle settings view
        if (current_view == "settings")
        {
            if (event == Event::Escape)
            {
                current_view = "list";
                status_message = "Settings closed.";
                return true;
            }
            else if (event == Event::Character('c'))
            {
                show_completed = !show_completed;
                refresh_tasks();
                status_message = show_completed ? "Now showing all tasks" : "Now showing active tasks only";
                screen.PostEvent(Event::Custom);
                return true;
            }
            return true;
        }
        
        // Handle other views (help, AI suggestions)
        if (current_view != "list")
        {
            // Return to list view on any key
            current_view = "list";
            return true;
        }

        // Main list view controls
        if (event == Event::Character('q') || event == Event::Escape)
        {
            screen.ExitLoopClosure()();
            return true;
        }
        else if (event == Event::Character('a'))
        {
            add_task_dialog();
            return true;
        }
        else if (event == Event::Character('e'))
        {
            edit_task_dialog();
            return true;
        }
        else if (event == Event::Character('d'))
        {
            delete_task_dialog();
            return true;
        }
        else if (event == Event::Character('t'))
        {
            add_subtask_dialog();
            return true;
        }
        else if (event == Event::Character(' '))
        {
            toggle_task_completion();
            return true;
        }
        else if (event == Event::Character('s'))
        {
            show_ai_suggestions();
            return true;
        }
        else if (event == Event::Character('S'))
        {
            show_schedule_summary();
            return true;
        }
        else if (event == Event::Character('g'))
        {
            show_settings();
            return true;
        }
        else if (event == Event::Character('G'))
        {
            sync_to_sheets();
            return true;
        }
        else if (event == Event::Character('c'))
        {
            show_completed = !show_completed;
            refresh_tasks();
            status_message = show_completed ? "Showing all tasks" : "Showing active tasks only";
            return true;
        }
        else if (event == Event::Character('h'))
        {
            show_help();
            return true;
        }
        else if (event == Event::ArrowUp)
        {
            if (selected_index > 0)
            {
                selected_index--;
            }
            return true;
        }
        else if (event == Event::ArrowDown)
        {
            if (selected_index < static_cast<int>(tasks.size()) - 1)
            {
                selected_index++;
            }
            return true;
        }

        return false; });

    auto renderer = Renderer(component, [&]
                             { return render(); });

    screen.Loop(renderer);
}

void TaskListView::edit_task_dialog()
{
    if (tasks.empty() || selected_index >= static_cast<int>(tasks.size()))
    {
        status_message = "No task selected.";
        return;
    }

    const Task &task = tasks[selected_index];

    // Populate input fields with current task data
    input_description = task.description;
    input_priority = task.priority;
    input_due_date = task.due_date.has_value() ? task.get_due_date_string() : "";
    input_link = task.links.empty() ? "" : task.links[0];
    input_progress = task.progress;
    current_input_field = 0;

    current_view = "edit";
    status_message = "Edit task (Tab to switch fields, ESC to cancel, Enter to save)";
}

void TaskListView::export_to_sheets()
{
    status_message = "This function is deprecated. Use 'G' to sync to Google Sheets.";
}

void TaskListView::add_subtask_dialog()
{
    if (tasks.empty() || selected_index >= static_cast<int>(tasks.size()))
    {
        status_message = "No task selected.";
        return;
    }

    const Task &parent = tasks[selected_index];

    // Reset input fields
    input_description = "";
    input_priority = parent.priority; // Inherit parent priority
    input_due_date = "";
    input_link = "";
    input_progress = 0;
    current_input_field = 0;

    current_view = "add_subtask";
    status_message = "Adding subtask to: \"" + parent.description + "\" (ESC to cancel, Enter to save)";
}

void TaskListView::show_settings()
{
    current_view = "settings";
    status_message = "Settings: [c] Toggle show completed | [ESC] Close";
}

void TaskListView::sync_to_sheets()
{
    status_message = "Google Sheets sync requires spreadsheet_id in config. Feature coming soon!";
    // TODO: Complete implementation with ConfigManager reference
    // show_progress = true;
    // progress_value = 0;
    // progress_message = "Syncing to Google Sheets...";
    // screen.PostEvent(Event::Custom);
    //
    // // Get all tasks
    // auto all_tasks = db.get_all_tasks(true);
    //
    // // Use GoogleSheets class to export
    // ConfigManager config;
    // config.load_config("config.json");
    // GoogleSheets sheets(config);
    //
    // if (!sheets.is_available())
    // {
    //     status_message = "Google Sheets is not enabled in config.json";
    //     show_progress = false;
    //     return;
    // }
    //
    // string spreadsheet_id = config.get_google_sheets_spreadsheet_id();
    // bool success = sheets.export_tasks(all_tasks, spreadsheet_id);
    //
    // if (success)
    // {
    //     status_message = "Successfully synced " + to_string(all_tasks.size()) + " tasks to Google Sheets!";
    // }
    // else
    // {
    //     status_message = "Failed to sync: " + sheets.get_last_error();
    // }
    //
    // show_progress = false;
    // screen.PostEvent(Event::Custom);
}
