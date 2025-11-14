#include "DatabaseManager.hpp"
#include <iostream>
#include <sstream>
#include <SQLiteCpp/SQLiteCpp.h>

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::make_unique;
using std::stringstream;

DatabaseManager::DatabaseManager(const string &db_path)
    : db_path(db_path)
{
    try
    {
        db = make_unique<SQLite::Database>(
            db_path,
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        cout << "Database opened successfully: " << db_path << endl;
    }
    catch (const exception &e)
    {
        cerr << "Error opening database: " << e.what() << endl;
        throw;
    }
}

void DatabaseManager::initilize_database()
{
    try
    {
        // Create tasks table
        db->exec(
            "CREATE TABLE IF NOT EXISTS tasks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "description TEXT NOT NULL, "
            "is_completed INTEGER NOT NULL DEFAULT 0, "
            "priority INTEGER NOT NULL DEFAULT 0, "
            "created_at INTEGER NOT NULL, "
            "due_date INTEGER, "
            "parent_id INTEGER, "
            "progress INTEGER NOT NULL DEFAULT 0, "
            "FOREIGN KEY (parent_id) REFERENCES tasks(id) ON DELETE CASCADE"
            ");");

        // Create task_links table for storing multiple links per task
        db->exec(
            "CREATE TABLE IF NOT EXISTS task_links ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "task_id INTEGER NOT NULL, "
            "link TEXT NOT NULL, "
            "FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE"
            ");");

        // Create tags table
        db->exec(
            "CREATE TABLE IF NOT EXISTS tags ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT UNIQUE NOT NULL"
            ");");

        // Create task_tags junction table
        db->exec(
            "CREATE TABLE IF NOT EXISTS task_tags ("
            "task_id INTEGER NOT NULL, "
            "tag_id INTEGER NOT NULL, "
            "PRIMARY KEY (task_id, tag_id), "
            "FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE, "
            "FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE"
            ");");

        // Create indices for better performance
        db->exec("CREATE INDEX IF NOT EXISTS idx_tasks_priority ON tasks(priority);");
        db->exec("CREATE INDEX IF NOT EXISTS idx_tasks_due_date ON tasks(due_date);");
        db->exec("CREATE INDEX IF NOT EXISTS idx_tasks_parent_id ON tasks(parent_id);");
        db->exec("CREATE INDEX IF NOT EXISTS idx_task_links_task_id ON task_links(task_id);");

        // Migration: Add progress column if it doesn't exist (for existing databases)
        try
        {
            SQLite::Statement check(*db, "SELECT progress FROM tasks LIMIT 1");
        }
        catch (const exception &)
        {
            // Column doesn't exist, add it
            cout << "Migrating database: Adding progress column..." << endl;
            db->exec("ALTER TABLE tasks ADD COLUMN progress INTEGER NOT NULL DEFAULT 0;");
            cout << "Migration complete." << endl;
        }

        // Migration: Add status column if it doesn't exist
        try
        {
            SQLite::Statement check(*db, "SELECT status FROM tasks LIMIT 1");
        }
        catch (const exception &)
        {
            cout << "Migrating database: Adding status column..." << endl;
            db->exec("ALTER TABLE tasks ADD COLUMN status INTEGER NOT NULL DEFAULT 0;");
            cout << "Migration complete." << endl;
        }

        cout << "Database initialized successfully." << endl;
    }
    catch (const exception &e)
    {
        cerr << "Error initializing database: " << e.what() << endl;
        throw;
    }
}

int DatabaseManager::add_task(const Task &task)
{
    try
    {
        SQLite::Statement query(*db,
                                "INSERT INTO tasks (description, is_completed, priority, created_at, due_date, parent_id, progress, status) "
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

        query.bind(1, task.description);
        query.bind(2, task.is_completed ? 1 : 0);
        query.bind(3, task.priority);
        query.bind(4, static_cast<int64_t>(task.created_at));

        if (task.due_date.has_value())
        {
            query.bind(5, static_cast<int64_t>(task.due_date.value()));
        }
        else
        {
            query.bind(5); // NULL
        }

        if (task.parent_id.has_value())
        {
            query.bind(6, task.parent_id.value());
        }
        else
        {
            query.bind(6); // NULL
        }

        query.bind(7, task.progress);
        query.bind(8, task.status);

        query.exec();

        int task_id = static_cast<int>(db->getLastInsertRowid());

        // Add links
        for (const auto &link : task.links)
        {
            add_task_link(task_id, link);
        }

        return task_id;
    }
    catch (const exception &e)
    {
        cerr << "Error adding task: " << e.what() << endl;
        return -1;
    }
}

vector<Task> DatabaseManager::get_all_tasks(bool include_completed)
{
    vector<Task> tasks;

    try
    {
        string query_str = "SELECT id, description, is_completed, priority, created_at, due_date, parent_id, progress, status FROM tasks";

        if (!include_completed)
        {
            query_str += " WHERE is_completed = 0";
        }

        query_str += " ORDER BY priority DESC, due_date ASC";

        SQLite::Statement query(*db, query_str);

        while (query.executeStep())
        {
            Task task;
            task.id = query.getColumn(0).getInt();
            task.description = query.getColumn(1).getText();
            task.is_completed = query.getColumn(2).getInt() != 0;
            task.priority = query.getColumn(3).getInt();
            task.created_at = static_cast<time_t>(query.getColumn(4).getInt64());

            if (!query.getColumn(5).isNull())
            {
                task.due_date = static_cast<time_t>(query.getColumn(5).getInt64());
            }

            if (!query.getColumn(6).isNull())
            {
                task.parent_id = query.getColumn(6).getInt();
            }

            task.progress = query.getColumn(7).getInt();
            task.status = query.getColumn(8).getInt();

            // Load links
            task.links = get_task_links(task.id);

            tasks.push_back(task);
        }
    }
    catch (const exception &e)
    {
        cerr << "Error getting all tasks: " << e.what() << endl;
    }

    return tasks;
}

optional<Task> DatabaseManager::get_task_by_id(int task_id)
{
    try
    {
        SQLite::Statement query(*db,
                                "SELECT id, description, is_completed, priority, created_at, due_date, parent_id, progress, status "
                                "FROM tasks WHERE id = ?");

        query.bind(1, task_id);

        if (query.executeStep())
        {
            Task task;
            task.id = query.getColumn(0).getInt();
            task.description = query.getColumn(1).getText();
            task.is_completed = query.getColumn(2).getInt() != 0;
            task.priority = query.getColumn(3).getInt();
            task.created_at = static_cast<time_t>(query.getColumn(4).getInt64());

            if (!query.getColumn(5).isNull())
            {
                task.due_date = static_cast<time_t>(query.getColumn(5).getInt64());
            }

            if (!query.getColumn(6).isNull())
            {
                task.parent_id = query.getColumn(6).getInt();
            }

            task.progress = query.getColumn(7).getInt();
            task.status = query.getColumn(8).getInt();

            // Load links
            task.links = get_task_links(task.id);

            return task;
        }
    }
    catch (const exception &e)
    {
        cerr << "Error getting task by ID: " << e.what() << endl;
    }

    return std::nullopt;
}

bool DatabaseManager::update_task(const Task &task)
{
    try
    {
        SQLite::Statement query(*db,
                                "UPDATE tasks SET description = ?, is_completed = ?, priority = ?, "
                                "due_date = ?, parent_id = ?, progress = ?, status = ? WHERE id = ?");

        query.bind(1, task.description);
        query.bind(2, task.is_completed ? 1 : 0);
        query.bind(3, task.priority);

        if (task.due_date.has_value())
        {
            query.bind(4, static_cast<int64_t>(task.due_date.value()));
        }
        else
        {
            query.bind(4); // NULL
        }

        if (task.parent_id.has_value())
        {
            query.bind(5, task.parent_id.value());
        }
        else
        {
            query.bind(5); // NULL
        }

        query.bind(6, task.progress);
        query.bind(7, task.status);
        query.bind(8, task.id);

        query.exec();

        return true;
    }
    catch (const exception &e)
    {
        cerr << "Error updating task: " << e.what() << endl;
        return false;
    }
}

bool DatabaseManager::delete_task(int task_id)
{
    try
    {
        SQLite::Statement query(*db, "DELETE FROM tasks WHERE id = ?");
        query.bind(1, task_id);
        query.exec();
        return true;
    }
    catch (const exception &e)
    {
        cerr << "Error deleting task: " << e.what() << endl;
        return false;
    }
}

vector<Task> DatabaseManager::get_tasks_by_priority(int priority)
{
    vector<Task> tasks;

    try
    {
        SQLite::Statement query(*db,
                                "SELECT id, description, is_completed, priority, created_at, due_date, parent_id, progress, status "
                                "FROM tasks WHERE priority = ? ORDER BY due_date ASC");

        query.bind(1, priority);

        while (query.executeStep())
        {
            Task task;
            task.id = query.getColumn(0).getInt();
            task.description = query.getColumn(1).getText();
            task.is_completed = query.getColumn(2).getInt() != 0;
            task.priority = query.getColumn(3).getInt();
            task.created_at = static_cast<time_t>(query.getColumn(4).getInt64());

            if (!query.getColumn(5).isNull())
            {
                task.due_date = static_cast<time_t>(query.getColumn(5).getInt64());
            }

            if (!query.getColumn(6).isNull())
            {
                task.parent_id = query.getColumn(6).getInt();
            }

            task.progress = query.getColumn(7).getInt();
            task.status = query.getColumn(8).getInt();

            task.links = get_task_links(task.id);
            tasks.push_back(task);
        }
    }
    catch (const exception &e)
    {
        cerr << "Error getting tasks by priority: " << e.what() << endl;
    }

    return tasks;
}

vector<Task> DatabaseManager::get_overdue_tasks()
{
    vector<Task> tasks;

    try
    {
        time_t now = time(nullptr);
        SQLite::Statement query(*db,
                                "SELECT id, description, is_completed, priority, created_at, due_date, parent_id, progress, status "
                                "FROM tasks WHERE due_date IS NOT NULL AND due_date < ? AND is_completed = 0 "
                                "ORDER BY due_date ASC");

        query.bind(1, static_cast<int64_t>(now));

        while (query.executeStep())
        {
            Task task;
            task.id = query.getColumn(0).getInt();
            task.description = query.getColumn(1).getText();
            task.is_completed = query.getColumn(2).getInt() != 0;
            task.priority = query.getColumn(3).getInt();
            task.created_at = static_cast<time_t>(query.getColumn(4).getInt64());

            if (!query.getColumn(5).isNull())
            {
                task.due_date = static_cast<time_t>(query.getColumn(5).getInt64());
            }

            if (!query.getColumn(6).isNull())
            {
                task.parent_id = query.getColumn(6).getInt();
            }

            task.progress = query.getColumn(7).getInt();
            task.status = query.getColumn(8).getInt();
            task.links = get_task_links(task.id);
            tasks.push_back(task);
        }
    }
    catch (const exception &e)
    {
        cerr << "Error getting overdue tasks: " << e.what() << endl;
    }

    return tasks;
}

vector<Task> DatabaseManager::get_subtasks(int parent_id)
{
    vector<Task> tasks;

    try
    {
        SQLite::Statement query(*db,
                                "SELECT id, description, is_completed, priority, created_at, due_date, parent_id, progress, status "
                                "FROM tasks WHERE parent_id = ? ORDER BY priority DESC");

        query.bind(1, parent_id);

        while (query.executeStep())
        {
            Task task;
            task.id = query.getColumn(0).getInt();
            task.description = query.getColumn(1).getText();
            task.is_completed = query.getColumn(2).getInt() != 0;
            task.priority = query.getColumn(3).getInt();
            task.created_at = static_cast<time_t>(query.getColumn(4).getInt64());

            if (!query.getColumn(5).isNull())
            {
                task.due_date = static_cast<time_t>(query.getColumn(5).getInt64());
            }

            if (!query.getColumn(6).isNull())
            {
                task.parent_id = query.getColumn(6).getInt();
            }

            task.progress = query.getColumn(7).getInt();
            task.status = query.getColumn(8).getInt();

            task.links = get_task_links(task.id);
            tasks.push_back(task);
        }
    }
    catch (const exception &e)
    {
        cerr << "Error getting subtasks: " << e.what() << endl;
    }

    return tasks;
}

bool DatabaseManager::add_task_link(int task_id, const string &link)
{
    try
    {
        SQLite::Statement query(*db,
                                "INSERT INTO task_links (task_id, link) VALUES (?, ?)");

        query.bind(1, task_id);
        query.bind(2, link);
        query.exec();

        return true;
    }
    catch (const exception &e)
    {
        cerr << "Error adding task link: " << e.what() << endl;
        return false;
    }
}

vector<string> DatabaseManager::get_task_links(int task_id)
{
    vector<string> links;

    try
    {
        SQLite::Statement query(*db,
                                "SELECT link FROM task_links WHERE task_id = ?");

        query.bind(1, task_id);

        while (query.executeStep())
        {
            links.push_back(query.getColumn(0).getText());
        }
    }
    catch (const exception &e)
    {
        cerr << "Error getting task links: " << e.what() << endl;
    }

    return links;
}
